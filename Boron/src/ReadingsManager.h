#ifndef _READINGS_H_
#define _READINGS_H_
#include "Sensor.h"
#include "UltrasonicSensor.h"
#include "CommonDataTypes.h"

class ReadingsManager {
  private:
    const int PERIPHERAL_ENABLE_PIN = D3; //For PCB this need to change to D3.
    const int MAXBOTIX_INPUT_PIN = A1;
    const int TEMP_INPUT_PIN = A2;
    int samples = 20; //Number of samples we use to avg our data.
    SensorConfig workingConfig;
    AnalogUltrasonicSensor *maxbotixSensor;

    float oldAnalog = ProcessOldAnalog();
    float oldAnalogAvg = ProcessOldAnalogAvg();
    float newAnalog = ProccessNewAnalog();
    float newAnalogAvg = ProccessNewAnalogAvg();
    float serialData = ProcessSerial();
    float tempData = processTemp(); 
    
  std::vector<SensorConfig> sensors; 

  public:
    ReadingsManager() {
      //Single config needed to save space.  It is copied into the vector with
      //push_back and can be rewritten safely.
      //Note: Perform ALL setup for sensors here.  If the variable type is
      //lost, you CANNOT recover it.
      //This means that, after setup and storing the value in "sensors," you can
      //only call functions available in the Sensors.h file.  For example, you
      //cannot call Configure() in the maxbotix sensor after storing it into a
      //SensorConfig.  If you can still access the underlying object, like with
      //the BMP280 sensors and FuelGauge, this does not matter so much.
          
      maxbotixSensor = new AnalogUltrasonicSensor(String("WaterLevelSensor"), MAXBOTIX_INPUT_PIN);
      maxbotixSensor->Configure(3.3, 120 * 2.54, 4096); // 12 bit, 3.3V ADC.  Sensor is ~0.1V per foot.
      workingConfig.SensorPtr = maxbotixSensor;
      workingConfig.NumSamples = 10;
      workingConfig.SamplePeriod_ms = 100;
      sensors.push_back(workingConfig);

      //Register sensors names with the buffer.
      for (unsigned int i = 0; i < sensors.size(); ++i) {
        Buffering::RegisterSensorName(sensors.at(i).SensorPtr->GetName());
      }
    }
    String dataInfoToString();
    void updateReadings();
    void setSamplesForAvg(int var); //Function to change sumber of samples taken.
    float ProcessOldAnalog();
    float ProcessOldAnalogAvg();
    float ProccessNewAnalog();
    float ProccessNewAnalogAvg();
    float ProcessSerial();
    float processTemp();
};

//Used to format sensor data as internal JSON variables.
String ReadingsManager::dataInfoToString() {
  /*Legacy code there was 
  Variable that were passed into this function: oldAnalog, oldAnalogAvg,    newAnalog,    newAnalogAvg,       serialData,        tempData
  The function prototypes that are were used:   float old, float newAnalog, float oldAvg, float newAnalogAvg, float serialValue, float temp
  Particle.Publish returns bool depending on if it publishes or not.
  String msg = String::format("{\"id\":" + sensorID + " , \"wl1\": %0.5f, \"wl2\": %0.5f, \"wl3\": %0.2f, \"temp\": %0.2f, \"b\": %0.2f}",oldAvg, newAnalog, newAnalogAvg, temp, fuelGauge.getSoC());*/
  String msg = "";
  msg += String::format("\"wl1\": %0.5f", oldAnalogAvg);
  msg += String::format(",\"wl2\": %0.5f", newAnalog);
  msg += String::format(",\"wl3\": %0.2f", newAnalogAvg);
  msg += String::format(",\"temp\": %0.2f", tempData);
  return msg;
}

void ReadingsManager::updateReadings() {
  Serial.println("Updating Readings");
  //Drive trigger pin low to turn on Maxbotix.
  digitalWrite(PERIPHERAL_ENABLE_PIN, LOW);
  //delay(500);
  //Grab all of our data while we are connecting to the internet.
  oldAnalog = ProcessOldAnalog();
  oldAnalogAvg = ProcessOldAnalogAvg();
  newAnalog = ProccessNewAnalog();
  newAnalogAvg = ProccessNewAnalogAvg();
  serialData = ProcessSerial();
  tempData = processTemp(); 
  //Drive trigger pin high to turn off maxbotix.
  digitalWrite(PERIPHERAL_ENABLE_PIN, HIGH);
}

void ReadingsManager::setSamplesForAvg(int var){
  samples = var;
}

float ReadingsManager::processTemp() {
  //TEMP_INPUT_PIN
  const int anVolt = analogRead(TEMP_INPUT_PIN);
  double convert = 0.8056640625;
  float mV = anVolt * convert;
  Serial.print("\tconv = ");
  Serial.println(convert);
  Serial.print("\tanV = ");
  Serial.println(anVolt);
  Serial.print("\tmV = ");
  Serial.println(mV);
  //Degree C.
  float tc = ((mV - 500) / 10);
  Serial.print("\ttemp C = ");
  Serial.println(tc);
  //Degree F.
  float tf = (tc * 1.8) + 32;
  Serial.print("\ttemp f = ");
  Serial.println(tf);
  return tf;
}

float ReadingsManager::ProcessOldAnalog() {
  for (unsigned int i = 0; i < sensors.size(); ++i) {
    SensorConfig sc = sensors.at(i);
    Sensor *s = sc.SensorPtr;
    //time_t startTime = Time.now();
    //Sensor returns cm , multiplu by 0.393701 for inches.
    return (s->GetSample() * 0.393701);
  }
  return 0;
}

float ReadingsManager::ProcessOldAnalogAvg() {
  for (unsigned int i = 0; i < sensors.size(); ++i) {
    SensorConfig sc = sensors.at(i);
    Sensor *s = sc.SensorPtr;
    time_t startTime = Time.now();
    //0.393701 * cm = reading in inches.
    float sensorOutput = s->GetSample() * 0.393701;
    float data2 = 0;
    Serial.print("\tdata = ");
    Serial.println(sensorOutput);
    for (int i = 0; i < samples; i++) {
      //Get our reading.
      data2 += s->GetSample();
      //Wait one second and take another reading.
      //delay(100);
    }
    //Average our reading.
    float data2avg = data2 / samples;
    float inchesavg2 = data2avg * 0.393701;
    Serial.print("\tData Avg: ");
    Serial.println(inchesavg2);
    return (inchesavg2);
  }
  return 0;
}

float ReadingsManager::ProccessNewAnalog() {
  const int anVolt = analogRead(MAXBOTIX_INPUT_PIN);
  //Maxbotix scale is Vcc/1024 V/cm, Particle reads at 4095.
  //In order to get the correct scale, we must divide by ~4.
  float cm = anVolt / 4;
  float inches = cm * 0.393701;
  Serial.print("\tAnalog In : ");
  Serial.println(inches);
  float cm2 = 0;
  //Conduct 10 measurements to improve accuracy
  for (int i = 0; i < 10; i++) {
    cm2 += analogRead(MAXBOTIX_INPUT_PIN) / 4; //Retrieve reading
    // wait one second and take another reading
    //delay(100);
  }
  //Average the measurements 
  float cmavg = cm2 / 10;
  float inchesavg = cmavg * 0.393701;
  Serial.print("\tAnalog In avgd: ");
  Serial.println(inchesavg);
  return (inches);
}

float ReadingsManager::ProccessNewAnalogAvg() {
  const int anVolt = analogRead(MAXBOTIX_INPUT_PIN);
  //Maxbotix scale is Vcc/1024 V/cm, Particle reads at 4095.
  //In order to get the correct scale we must divide by ~4.
  float cm = anVolt / 4;
  float inches = cm * 0.393701;
  Serial.print("\tAnalog In : ");
  Serial.println(inches);
  float cm2 = 0;
  //Conduct 10 measurements to improve accuracy
  for (int i = 0; i < samples; i++) {
    cm2 += analogRead(MAXBOTIX_INPUT_PIN) / 4; //Retrieve reading
    // wait one second and take another reading
    //delay(100);
  }
  //Average the measurements 
  float cmavg = cm2 / samples;
  float inchesavg = cmavg * 0.393701;
  Serial.print("\tAnalog In avgd: ");
  Serial.println(inchesavg);
  return (inchesavg);
}

float ReadingsManager::ProcessSerial() {
  char buf[7];
  static uint32_t msTimeout = 0; //Timestamp of recently found R.
                                 //Persists between calls of function.
  float distanceToWaterMM = 0;
  while (Serial1.available()) {
    if ('R' == Serial1.peek()) {
      if (!msTimeout) {
        msTimeout = millis(); //When new R found - store timestamp.
      }
      if (Serial1.available() >= 5) { //Check if complete packet has been recieved.
        memset(buf, 0x00,
               sizeof(buf));                  //Ensure properly terminated string.
        Serial1.readBytes(buf, 6);            //Read the packet.
        distanceToWaterMM = atoi(&buf[1]);    //Convert string following the R to integer.
        msTimeout = 0;                        //Prepare for next incoming R.
      } else if (millis() - msTimeout > 50) { //Current R is outdated.
        Serial1.read();                       //Flush R and continue.
        msTimeout = 0;                        //Prepare for next incoming R.
      }
    } else {
      Serial1.read();
      //Serial1.read(); // flush "orphaned" bytes
    }
  }
  Serial.print("\tSerial Output : ");
  Serial.println((distanceToWaterMM * 0.393701) / 10);
  return ((distanceToWaterMM * 0.393701) / 10);
}
#endif  
