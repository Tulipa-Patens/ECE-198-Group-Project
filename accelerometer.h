#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// --------------------------------------------------
// Sport types
// --------------------------------------------------

enum SportType {
  SPORT_ARM_RAINBOWS,
  SPORT_ARNOLD_PRESS,
  SPORT_BICEPS_CURLS,
  SPORT_UNKNOWN
};

// --------------------------------------------------
// Initialization and control
// --------------------------------------------------

bool initializeAccelerometer();

void start_acc(SportType sport);
void stop_acc();

bool isAccelerometerRunning();

// --------------------------------------------------
// Sport selection
// --------------------------------------------------

SportType getCurrentSport();
SportType parseSportType(const String &name);
const __FlashStringHelper *getSportName(SportType sport);

// --------------------------------------------------
// Sensor processing
// --------------------------------------------------

void updateSensorData();
void updateMagnitude();
void updateFrequencyAndRepetitionCount();

// --------------------------------------------------
// Analysis
// --------------------------------------------------

void analyze(SportType sport);
void analyzeArmRainbows();
void analyzeArnoldPress();
void analyzeBicepsCurls();

bool isMovementValid();

// --------------------------------------------------
// Reset functions
// --------------------------------------------------

void resetAnalysis();
void resetRepetitionCount();

// --------------------------------------------------
// Accelerometer and gyroscope getters
// --------------------------------------------------

float getLinearMagnitude();
float getAngularMagnitude();

float getAccelerationX();
float getAccelerationY();
float getAccelerationZ();

float getGyroscopeX();
float getGyroscopeY();
float getGyroscopeZ();

#endif