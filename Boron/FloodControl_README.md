# Flood Control - Sensor

## Abstract

This code is used to operate a custom-build water level sensor for the purpose of having early-warning detection against floods. The sensor is built using a Paricle Boron LTE CAT-M1/NB1 Development Board that is inserted into a custom platform which connects to an analog hypersonic sonar detector, a solar charger, a small battery, and a Taoglas FXUB63 Ultra Wide Band Antenna (698-3000 MHz). The code in this repository acts as the driver for the Boron device, allowing it to record the water level, send data over an LTE connection to a database, and controlling its activity time to conserve power.
The code in this repositiory is built off of previous code built for the previous generation of Boron Electron controllers. The migration to Boron is an attempt to stay ahead of the curve of technology, as well as improve on previous workflows and make the device more efficient, as well as fixing critical glitches that prevent the device from working as intended.

## Hardware

- Particle Boron LTE CAT-M1/NB1 Development Board
- 103450 Particle 3.7V 1800 mAh 6.66 WH Rechargable Battery
- Custom-built Tennessee Tech-provided Circuit Board
- Custom-built Tennessee Tech-provided Analog Hypersonic Sonar Sensor
- Taoglas FXUB63 Ultra Wide Band Antenna (698-3000 MHz) LTE Antenna

## Code repository

### Boron.ino/.cpp

### AnalogUltrasonicSensor.h

### CommonDataTypes.h

### RetainedBufferSimpler.h

### Sensor.h

### StatsArray.h

### StatsTools.h

### UltrasonicSensor.h/.cpp
