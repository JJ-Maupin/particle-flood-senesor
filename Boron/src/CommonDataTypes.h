#ifndef _COMMON_DATA_TYPES_H_
#define _COMMON_DATA_TYPES_H_
// Authored by Chris Stringer, Edited by Dakota Aaron

// Our packet IDs tucked into a simple struct
struct packetID {
  uint8_t maxbotics = 0x01;
  uint8_t timeSync = 0x02;
  uint8_t sendData = 0x04;
  uint8_t stopData = 0x05;
  uint8_t stopDataResp = 0x06;
  uint8_t sleepFlag = 0x07;
  uint8_t sleepResp = 0x08;
  uint8_t uCresp = 0xB;
  uint8_t netSync = 0xC;
};

#include "Sensor.h"

struct SensorData {
  //
  String Name;
  float Value;
  time_t Timestamp;
  float SOC;
  String nodeID;
};

// Hold both a sensor reference and information required for sampling
struct SensorConfig {
  Sensor *SensorPtr;   // Can be a subclass as well
  int NumSamples;      // How many samples to take for this sensor
  int SamplePeriod_ms; // How long between each sample to wait before taking another.
                       // This approximates a sampling interval if this value is long
                       // compared to the sensor actuation time.
};

// Determine in what ways to send the data from the node
enum OutputLevel {
  NoOutput,
  LCD_Only,
  SerialOnly,
  Publish,
  LocalOnly, // Serial and LCD only
  NoLCD,     // Serial and Publish
  AllOutputs
};

#endif // _COMMON_DATA_TYPES_H_
