#ifndef _SENSOR_H_
#define _SENSOR_H_
// edited on 6/8/18 by dakota aaron for arudino compatiability
#include "StatsArray.h"

class Sensor {
public:
  Sensor(String name, String units) {
    name_ = new char[name.length() + 2];        // Make space for the name
    name.toCharArray(name_, name.length() + 1); // Copy name to memory
    name_[name.length() + 1] = 0;               // Add in Null Terminator

    units_ = new char[units.length() + 2];         // Make space for the name
    units.toCharArray(units_, units.length() + 1); // Copy name to memory
    units_[units.length() + 1] = 0;                // Add in Null Terminator
  };

  ~Sensor() {
    delete[] name_; // Free up name memory
  }

  // Overload with custom sensor sampling code
  virtual float GetSample() { return (enabled_) ? 0 : -1; }

  // Run a number of trials and average the results
  virtual float GetTrialAverage(const int numSamples, const int samplePeriod_ms, const bool trimOutliers = true) {
    if (enabled_) {
      StatsTools::StatsArray<float> samples(numSamples);

      for (int i = 0; i < numSamples; ++i) {
        samples.Append(GetSample());
        delay(samplePeriod_ms);
      }
      if (trimOutliers) {
        Serial.println("Trimming");
        samples.TrimOutliers();
      }

      return samples.Average();
    } else {
      Serial.println("Didnt trimm1");
      return -1;
    }
  }

  virtual void Enable() { enabled_ = true; }

  virtual void Disable() { enabled_ = false; }

  String GetName() { return String(name_); }

  String GetUnits() { return String(units_); }

private:
  bool enabled_;
  char *name_;
  char *units_;
};

#endif // _SENSOR_H_
