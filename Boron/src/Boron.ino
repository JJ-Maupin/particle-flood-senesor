#include "AnalogUltrasonicSensor.h"
#include "CommonDataTypes.h"
#include "RetainedBufferSimpler.h"
#include "Sensor.h"
#include "ReadingsManager.h"
#include "SleepManager.h"
#include <vector>
#include <string>

//Set to SEMI_AUTOMATIC so we can control connection.
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);
//STARTUP(cellular_credentials_set("RESELLER", "", "", NULL)); //Comment if the SIM card is from Particle

FuelGauge fuelGauge;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
  Easy naming convention, this triggers a webhook that will dump all data into chords for post analysis in graphana.
*/
String unitName = "Boron_WLTesting1";
String eventName = "Push";
String sensorID = "78";
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//declare harware pin layout
const int PERIPHERAL_ENABLE_PIN = D3;
const int MAXBOTIX_INPUT_PIN = A1;
const int TEMP_INPUT_PIN = A2;

int PERIOD = 15*60;  //(15min * 60sec) Changing this durations for nearby sensors to avoid crosstalk

const float MAX_ON_TIME_min = 3; //Maximum amount of minutes the unit is allowed to be on.
const float MAINTENANCE_DURATION_min = 10.0; //Length of Maintaince Period.

PMIC pmic; //Battery management.

SystemSleepConfiguration config;
ReadingsManager readingsMgr;      //Class for getting readings and managing data associated with them.
SleepManager slpMgr;              //Class for managging sleep and delay.
void allReadingsToServer();       //This function takes readings from the functions and adds them to the sensor buffer.
int weatherUpdater(String data);  //Polling rate event handler. 

//setup() runs once, when the device is first turned on.
//This is for code that must run once at startup. Do not delcare global variables here!
void setup() {
  //PMIC settings for optimum charging.
  pmic.begin();
  pmic.setChargeCurrent(0, 0, 1, 0, 0, 0);  //Set charging current to 1024mA (512 + 512 offset).
  pmic.setInputVoltageLimit(4840);          //Set the lowest input voltage to 4.84 volts. 
                                            //This keeps the 5v solar panel from operating below 4.84 volts.
  pmic.enableBuck();

  Serial.begin(9600); //Enable serial logging.
  
  //Serial for the maxbotix.
  Serial1.begin(9600);
  while (!Serial && !Serial1);
  delay(2000);

  Serial.print("");
  Serial.println("Serial Started");

  slpMgr.checkBatteryLevel(); //Check battery percentage.

  System.enableUpdates(); //Make sure updates are enabled.

  Particle.keepAlive(30); //Needed for AT&T Network.

  pinMode(PERIPHERAL_ENABLE_PIN, OUTPUT);

  BLE.off(); //Turn off bluetooth.

  Serial.println("Registering Functions");
  bool functionreg = Particle.function("weatherUpdater", weatherUpdater); //Register device to recieve events.
  while (!functionreg) {
    functionreg = Particle.function("weatherUpdater", weatherUpdater);
  }
  Serial.println("Setup Finished");
}

//loop() runs over and over again, as quickly as it can execute.
//Main program execution. Note that, if this completes, it will shut down and
//restart at setup(). It will not loop.
void loop() {
  bool skipupdate = false;
  Serial.println("Starting Itteration");
  //The core of your code will likely live here.
  slpMgr.checkBatteryLevel(); //Check battery
  Particle.connect(); //Connect to particle cloud
  
  //Verifiy stable network.
  Serial.println("Confirming Connection...");
  for (long t = millis() + 180000; millis() < t;) {
    if (Cellular.ready() && Particle.connected() && Network.ready()) {
      skipupdate = false;
      Serial.println("Connection Success!");
      break;
    }
  }

  if (!skipupdate) {
    Particle.syncTime(); //Sync the clock.
    unsigned long startTime = millis();
    Particle.process(); //Check for event updates.
    
    readingsMgr.updateReadings(); //Update current readings.
    allReadingsToServer(); //Post readings to server.
    
    //Before sleep see if we are on our maint time interval.
    //UTC is 6 hrs ahead of central so we must adjust our time.
    if (Time.hour() == 17 && Time.minute() <= 20) {
      //Wait and do nothing for maint duration.
      //NOTE: this will cause us to miss 1 reading.
      //Send message that maintainces has started.
      //Send in battery reading.
      unsigned long maintTimer = millis();
      String mainMess = String::format("Maintaince Mode started for %s", unitName);
      Particle.publish("alerts", mainMess, PRIVATE);
      delay(MAINTENANCE_DURATION_min * 60 * 1000 );
    } 
  }
  //Delay and deap sleep.
  slpMgr.sleepFunction();
}

//Function for posting sensor data particle.io events. 
void allReadingsToServer() {
  Serial.println("Posting readings to server.");
  String msg;
  msg += "{";
  msg += String::format("\"id\":" + sensorID);
  msg += "," + readingsMgr.dataInfoToString();
  msg += String::format(",\"b\": %0.2f", fuelGauge.getSoC());
  msg += "}";
  Serial.println("\t" + msg);

  //Attempt to publish data until the node times out.
  bool allSent = false;
  unsigned long startTime = millis();
  while (((millis() - startTime) / 30000.0 < MAX_ON_TIME_min) && Particle.connected() && !allSent) {
    //Currently no buffer, this will attempt to send the data until time out.
    allSent = Particle.publish(eventName, msg,WITH_ACK);
  }
}

//Callable function from Particle.io.
//Is used for changing the reading rate based off of the weather.
int weatherUpdater(String data) {
  Serial.println("function activated: " + data);
  bool inputcheck = true;

  //check for commands
  if (data == "maintenance") {
    slpMgr.allowSleep(false);
  } else if (data == "reset") {
    slpMgr.resetSleepPeriod();
  }

  //If not command check if only numbers.
  for (int i = 0; i < data.length(); i++) {
    if (!isDigit(data.charAt(i))) { //Make sure all character are ints.
      inputcheck = false;
    }
  }
  int ms = data.toInt();
  if (ms < 0) {
    inputcheck = false;
  } else if (ms < 130 * 1000) { //check if same as delay interval or functionaly similar i.e.(2min+10sec)
    ms = 0;
  }
  if (!inputcheck) {
    while (!Particle.publish(eventName, "{BAD INT for sleep}", PUBLIC, WITH_ACK )) { }
    return 0;
  }
  
  /Uupdate the sleep time and notify particle.io
  PERIOD = ms;
  slpMgr.setSleepPeriod(ms);
  Serial1.print("{Sleep time: ");
  Serial1.println(String((int)ms));
  while (!Particle.publish(eventName, "{SleepPeriod: " + String((int)PERIOD) + "}", PUBLIC, WITH_ACK )) { }
  
  return 0;
}
