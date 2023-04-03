
/*
 * Project Test
 * Description:
 * Author:
 * Date:
 */
#include "AnalogUltrasonicSensor.h"
#include "CommonDataTypes.h"
#include "RetainedBufferSimpler.h"
#include "Sensor.h"
#include <vector>
#include <string>


/////////////////////////////////Code Starts Below ///////////////////////////////////////////////////
// Set to SEMI_AUTOMATIC so we can control connection
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);
// STARTUP(cellular_credentials_set("RESELLER", "", "", NULL)); // Comment if the SIM card is from Particle - AJK 08/06/19

// these are not used currently
std::vector<SensorConfig> sensors;
std::vector<SensorData> sensorBuffer; // put into retained memory?

FuelGauge fuelGauge;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
  Easy naming convention, this will trigger a webhook that will dump all data into chords for post analysis in graphana
*/
String unitName = "Boron_WLTesting1";
// USE OS Version 2.1.0 which seems to work according to Note. AJK 6/8/21
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Configuration
const int PERIPHERAL_ENABLE_PIN = D3;
// for PCB this need to change to D3
const int MAXBOTIX_INPUT_PIN = A1;
const int TEMP_INPUT_PIN = A2;
// number of samples we use to avg our data
const int samples = 20;
//const float RUN_INTERVAL_min = 15.0;
int PERIOD = 15*60;  // (15min * 60sec) Changing this durations for nearby sensors to avoid crosstalk - AJK 7/20/21
long timeToInterval = 0;
// Maximum amount of minutes the unit is allowed to be on
const float MAX_ON_TIME_min = 3;
// Length of Maintaince Period
const float MAINTENANCE_DURATION_min = 10.0;



SystemSleepConfiguration config; 


// function prototypes
float ProcessOldAnalog();
float ProccessNewAnalog();
float ProcessOldAnalogAvg();
float ProccessNewAnalogAvg();
float ProcessSerial();
float processTemp();
time_t GetMidTimeStamp(time_t startTime, time_t stopTime);
long calculateSleepCycle();
// this function will take all readings from the various function, add them to
// the sensor buffer
bool allReadingsToServer(float old, float newAnalog, float oldAvg, float newAnalogAvg, float Serial);
// Battery protection function
void checkBattery(bool debug);
PMIC pmic;

void sleepfunction();



//polling rate event handeler. 
//WeatherUpdate weatherUpdate;
void weatherupdatehandler(const char *event, const char *data);
int weatherUpdater(String data);


// setup() runs once, when the device is first turned on.
void setup() {

  //System.enableUpdates();
  // Put initialization like pinMode and begin functions here.
  Serial.begin(9600);
   
  // for the maxbotix
  Serial1.begin(9600);
  
  Particle.keepAlive(30); // Needed for AT&T Network

  pinMode(PERIPHERAL_ENABLE_PIN, OUTPUT);

  // Single config needed to save space.  It is copied into the vector with
  // push_back and can be rewritten safely.
  SensorConfig workingConfig;

  //   Note: Perform ALL setup for sensors here.  If the variable type is
  //   lost, you CANNOT recover it.
  // This means that, after setup and storing the value in "sensors," you can
  // only call functions available in the Sensors.h file.  For example, you
  // cannot call Configure() in the maxbotix sensor after storing it into a
  // SensorConfig.  If you can still access the underlying object, like with
  // the BMP280 sensors and FuelGauge, this does not matter so much.

  // Maxbotix Sensor
  AnalogUltrasonicSensor *maxbotixSensor = new AnalogUltrasonicSensor(String(unitName), MAXBOTIX_INPUT_PIN);
  maxbotixSensor->Configure(3.3, 120 * 2.54, 4096); // 12 bit, 3.3V ADC.  Sensor is ~0.1V per foot.
  workingConfig.SensorPtr = maxbotixSensor;
  workingConfig.NumSamples = 10;
  workingConfig.SamplePeriod_ms = 100;
  sensors.push_back(workingConfig);

  // Register sensors names with the buffer
  for (unsigned int i = 0; i < sensors.size(); ++i) {
    Buffering::RegisterSensorName(sensors.at(i).SensorPtr->GetName());
  }

  // PMIC settings for charging
  // setting up PMIC for optimum charging
  pmic.begin();
  pmic.setChargeCurrent(0, 0, 1, 0, 0, 0); // Set charging current to 1024mA (512 + 512 offset)
  pmic.setInputVoltageLimit(4840);         // Set the lowest input voltage to 4.84 volts. This keeps my 5v
                                           // solar panel from operating below 4.84 volts.
  pmic.enableBuck();

  //turn off bluetooth
  BLE.off();

  //register device to recive event
  
  
  
  bool functionreg = Particle.function("weatherUpdater", weatherUpdater );

  while(!functionreg){
    functionreg = Particle.function("weatherUpdater", weatherUpdater );
  }

  
}


// loop() runs over and over again, as quickly as it can execute.
// Main program execution.  Note that, if this completes, it will shut down and
// restart at setup(). It will not loop.
//int secondsetuplock = 2147483647;
//bool subscribed = 0;
void loop() {
  // The core of your code will likely live here.
  
    
  
    
  

  // check battery
  checkBattery(false);


 
  // connect to particle cloud
  Particle.connect();
  // sync the clock`
  Particle.syncTime();
  unsigned long startTime = millis();
  Particle.publish(unitName, "Test", PRIVATE); // inserted this line on May 26, 2021 - AJK
  // drive trigger pin low to turn on Maxbotix
  digitalWrite(PERIPHERAL_ENABLE_PIN, LOW);
  //delay(500);
  // grab all of our data while we are connecting to the internet
  float oldAnalog = ProcessOldAnalog();
  /* AJK has started testing this code from 06/25/2021 to see if not calling the additional functions ProcessOldAnalogAvg(), ProccessNewAnalog(), ProcessSerial() and processTemp()
  will cause any improvements in the behavior of Borons, as the computational work is not reduced. */
  /*float oldAnalogAvg = oldAnalog;
  float newAnalog = oldAnalog;
  float newAnalogAvg = oldAnalog;
  float serialData = oldAnalog;
  float tempData = oldAnalog;*/
  float oldAnalogAvg = ProcessOldAnalogAvg();
  float newAnalog = ProccessNewAnalog();
  float newAnalogAvg = ProccessNewAnalogAvg();
  float serialData = ProcessSerial();
  float tempData = processTemp(); 
 /////////////////////////Get Charge Fault Register///////////
  byte a =0b11111111;
  Serial.println(a);
  a = pmic.getFault();
  Serial.println("charging");
  Serial.println(a);
  Serial.println("Fault");
////////////////////////////
  //delay(500);
  // drive trigger pin high to turn off maxbotix
  digitalWrite(PERIPHERAL_ENABLE_PIN, HIGH);

  // This will for connection and make sure the time is valid, else it will time out
  while (((millis() - startTime) / 60000.0 < MAX_ON_TIME_min) && !Particle.connected() && !Time.isValid()) {
    // do nothing
  }
  //delay(10000);
  // Attempt to publish data until the node times out
  bool allSent = false;
  while (((millis() - startTime) / 60000.0 < MAX_ON_TIME_min) && Particle.connected() && false == allSent) {
    // currently no buffer, this will attempt to send the data until time out
    allSent |= allReadingsToServer(oldAnalog, oldAnalogAvg, newAnalog, newAnalogAvg, serialData, tempData);
    // allSent |= allReadingsToServer(1.1, 2.2, 3.3, 4.4, 5.5, 6.6);  // AJK is testing to see if the data is being sent properly to Cloud - 06-23/2021
    Serial.println(allSent);
  }
  

  // before sleep see if we are on our maint time interval
  // UTC is 6 hrs ahead of central so we must adjust our time
  if (Time.hour() == 17 && Time.minute() <= 10) {
    // wait and do nothing for maint duration
    // NOTE this will cause us to miss 1 reading
    // send message that maintainces has started
    // send in battery reading
    unsigned long maintTimer = millis();
    String mainMess = String::format("Maintaince Mode started for %s", unitName);
    Particle.publish("alerts", mainMess, PRIVATE);
    while (((millis() - maintTimer) / 1000.0) < MAINTENANCE_DURATION_min * 60) {
      // do nothing
    }
    sleepfunction();

  } else {
    // disconnect from particle cloud
    //Particle.disconnect(); never disconnect as possible agressive reconnect
    while ((Particle.connected() && (millis() - startTime) / 60000.0 < MAX_ON_TIME_min)) {
      // wait until cloud operations are done and disconnect
    }
    sleepfunction();

  }


}
float processTemp() {
  // TEMP_INPUT_PIN
  const int anVolt = analogRead(TEMP_INPUT_PIN);
  double convert = 0.8056640625;
  float mV = anVolt * convert;
  Serial.print("conv = ");
  Serial.println(convert);
  Serial.print("anV = ");
  Serial.println(anVolt);
  Serial.print("mV = ");
  Serial.println(mV);
  // degree C
  float tc = ((mV - 500) / 10);
  Serial.print("temp C = ");
  Serial.println(tc);
  // degree F
  float tf = (tc * 1.8) + 32;
  Serial.print("temp f = ");
  Serial.println(tf);
  return tf;
}

float ProcessOldAnalog() {
  for (unsigned int i = 0; i < sensors.size(); ++i) {
    SensorConfig sc = sensors.at(i);
    Sensor *s = sc.SensorPtr;
    time_t startTime = Time.now();
    // sensor returns cm , multiplu by 0.393701 for inches
    return (s->GetSample() * 0.393701);
  }
  return 0;
}

float ProcessOldAnalogAvg() {
  for (unsigned int i = 0; i < sensors.size(); ++i) {
    SensorConfig sc = sensors.at(i);
    Sensor *s = sc.SensorPtr;
    time_t startTime = Time.now();
    // 0.393701 * cm = reading in inches
    float sensorOutput = s->GetSample() * 0.393701;
    float data2 = 0;
    Serial.print("data = ");
    Serial.println(sensorOutput);
    for (int i = 0; i < samples; i++) {
      // get our reading
      data2 += s->GetSample();
      // wait one second and take another reading
      //delay(100);
    }

    // average our reading
    float data2avg = data2 / samples;
    float inchesavg2 = data2avg * 0.393701;
    Serial.print("Data Avg: ");
    Serial.println(inchesavg2);
    return (inchesavg2);
  }
  return 0;
}

float ProccessNewAnalog() {
  const int anVolt = analogRead(MAXBOTIX_INPUT_PIN);
  // maxbotix scale is Vcc/1024 V/cm, Particle reads at 4095
  // in order to get the correct scale we must divide by ~4
  float cm = anVolt / 4;
  float inches = cm * 0.393701;
  Serial.print("Analog In : ");
  Serial.println(inches);
  float cm2 = 0;
  // lets do a 10 measurement avg
  for (int i = 0; i < 10; i++) {
    // get our reading
    cm2 += analogRead(MAXBOTIX_INPUT_PIN) / 4;
    // wait one second and take another reading
    //delay(100);
  }

  // average our reading
  float cmavg = cm2 / 10;
  float inchesavg = cmavg * 0.393701;
  Serial.print("Analog In avgd: ");
  Serial.println(inchesavg);
  return (inches);
}

float ProccessNewAnalogAvg() {
  const int anVolt = analogRead(MAXBOTIX_INPUT_PIN);
  // maxbotix scale is Vcc/1024 V/cm, Particle reads at 4095
  // in order to get the correct scale we must divide by ~4
  float cm = anVolt / 4;
  float inches = cm * 0.393701;
  Serial.print("Analog In : ");
  Serial.println(inches);
  float cm2 = 0;
  // lets do a 10 measurement avg
  for (int i = 0; i < samples; i++) {
    // get our reading
    cm2 += analogRead(MAXBOTIX_INPUT_PIN) / 4;
    // wait one second and take another reading
    //delay(100);
  }

  // average our reading
  float cmavg = cm2 / samples;
  float inchesavg = cmavg * 0.393701;
  Serial.print("Analog In avgd: ");
  Serial.println(inchesavg);
  return (inchesavg);
}

float ProcessSerial() {
  char buf[7];
  static uint32_t msTimeout = 0; // timestamp of recently found R
                                 // persists between calls of function
  float distanceToWaterMM = 0;
  while (Serial1.available()) {
    if ('R' == Serial1.peek()) {
      if (!msTimeout) {
        msTimeout = millis(); // when new R found - store timestamp
      }
      if (Serial1.available() >= 5) { // already received a complete packet?
        memset(buf, 0x00,
               sizeof(buf));                  // ensure properly terminated string
        Serial1.readBytes(buf, 6);            // read the packet
        distanceToWaterMM = atoi(&buf[1]);    // convert string following the R to integer
        msTimeout = 0;                        // prepare for next incoming R
      } else if (millis() - msTimeout > 50) { // current R is outdated
        Serial1.read();                       // flush R and move on
        msTimeout = 0;                        // prepare for next incoming R
      }
    } else {
      Serial1.read();
      // Serial1.read(); // flush "orphaned" bytes
    }
  }
  Serial.print("Serial Output : ");
  Serial.println((distanceToWaterMM * 0.393701) / 10);
  return ((distanceToWaterMM * 0.393701) / 10);
}

time_t GetMidTimeStamp(time_t startTime, time_t stopTime) { return startTime + (stopTime - startTime) / 2; }

bool allReadingsToServer(float old, float newAnalog, float oldAvg, float newAnalogAvg, float serialValue, float temp) {
  // we will recive all these readings and format them in a string and send
  // them over the network
  // Particle.Publish returns bool depending on if it publishes or not.
  String msg = String::format("{ \"wl1\": %0.5f, \"wl2\": %0.5f, \"wl3\": %0.2f, \"temp\": %0.2f, \"b\": %0.2f}",
                              oldAvg, newAnalog, newAnalogAvg, temp, fuelGauge.getSoC());

  // return Particle.publish(unitName, msg, PRIVATE);
  
  return Particle.publish(unitName, msg,WITH_ACK); // Added this code on 06/17/21 - AJK
}



void checkBattery(bool debug) {
  if (!debug) {
    // dont send to sleep
  } else {
    if (fuelGauge.getSoC() < 20) {
      // go to sleep for the cycle time
      sleepfunction();
    }
  }
}

void sleepfunction(){

   //System.sleep( {}, {}, calculateSleepCycle() );
   //Particle.publish("PollingChange","data=" ,WITH_ACK);
    config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(calculateSleepCycle());
    System.sleep(config);  // Added this line on May 26, 2021 - commented the line above. - AJK
    //System.sleep(SLEEP_MODE_DEEP, calculateSleepCycle());

}

// retuns the number of seconds to sleep on the 15th minute schedual
// this is only going to trigger on successfully communicated network
long calculateSleepCycle() {
  
  //900 - (Time.now() % 900)
  // // on return convert it to seconds
  // return (mins * 60) + secs;
  /*long secNow = Time.now();

  //long secInPeriod = secNow % PERIOD;

  long secTillNextPeriod = PERIOD - secInPeriod;
  Serial.println(secTillNextPeriod);
  Serial.println("sleeptime: ");
  Serial.println(sleeptime);

  //100
  long sleeptime = (secTillNextPeriod * 100) + sleeptime;*/
  if(PERIOD == 0){
    return 0;
  }

  /*
  | -> Interval
  T is time
  |----T----------------|
  */

  long secNow = Time.now();
  long timeaft0 = PERIOD - (secNow % PERIOD);
  timeToInterval = PERIOD - (timeaft0);
  
  //Create delay window for after reading interval
  if(timeaft0 + 1 < 20){
    delay(20 - timeaft0);
  }

  //create delay window for before reading interval
  if(timeToInterval < 20){
    delay((int)timeToInterval);
  }


  while(!Particle.publish(unitName, "{Sleep time: " + String((int)timeToInterval) + "}", PUBLIC, WITH_ACK )){}
  

  return (timeToInterval-9) * 1000;
  

} // end function

int weatherUpdater(String data){

    int sec = PERIOD;
    Serial.println("function activated: " + data);
    bool inputcheck = true;
    if(data == "maintance"){
      sec = 0;

    }

    for(int i = 0; i < data.length(); i++){

      if(!isDigit(data.charAt(i))){
        inputcheck = false;
      }
    }
    sec = data.toInt();
    
    if(sec < 0){
        inputcheck = false;
    }
    if(!inputcheck){
      while(!Particle.publish(unitName, "{BAD INT for sleep}", PUBLIC, WITH_ACK )){}
      return 0;
    }
    
    PERIOD = sec;
    Serial1.print("{Sleep time: ");
    Serial1.println(String((int)PERIOD));
    while(!Particle.publish(unitName, "{Sleep time: " + String((int)PERIOD) + "}", PUBLIC, WITH_ACK )){}
    
    return 0;

}