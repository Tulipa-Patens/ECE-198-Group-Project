#include "accelerometer.h"
#include <math.h>

// --------------------------------------------------
// Objects and variables defined in other files
// --------------------------------------------------

extern Adafruit_MPU6050 mpu;

// Defined in athlete_detector.ino
extern unsigned long lastSampleTimeUs;

// Repetition variables defined in athlete_detector.ino
extern unsigned long repetitionCount;
extern float repetitionFrequency;

// ROM variables defined in athlete_detector.ino
extern float angularPosition;
extern float currentROM;
extern float maximumROM;
extern float minimumAngle;
extern float maximumAngle;

// --------------------------------------------------
// Configuration
// --------------------------------------------------

const float GRAVITY = 9.80665f;

/*
 * Adafruit's MPU6050 library returns gyroscope values in rad/s.
 */
const float GYRO_THRESHOLD_RAD_S = 0.35f;

// Arm-rainbow validation limits
const float MIN_ARM_RAINBOW_ROM = 30.0f;
const float MAX_ARM_RAINBOW_ROM = 360.0f;

const float MIN_ARM_RAINBOW_FREQUENCY = 0.1f;
const float MAX_ARM_RAINBOW_FREQUENCY = 5.0f;

/*
 * Arnold-press and biceps-curl validation limits.
 * These are starting values; tune them with real recorded data.
 */
const float MIN_ARNOLD_PRESS_ROM = 60.0f;
const float MAX_ARNOLD_PRESS_ROM = 360.0f;

const float MIN_ARNOLD_PRESS_FREQUENCY = 0.1f;
const float MAX_ARNOLD_PRESS_FREQUENCY = 1.5f;

const float MIN_BICEPS_CURL_ROM = 40.0f;
const float MAX_BICEPS_CURL_ROM = 180.0f;

const float MIN_BICEPS_CURL_FREQUENCY = 0.1f;
const float MAX_BICEPS_CURL_FREQUENCY = 2.0f;

// --------------------------------------------------
// Accelerometer-owned variables
// --------------------------------------------------

bool accelerometerRunning = false;

SportType currentSport = SPORT_UNKNOWN;

bool movementValid = false;

float accelerationX = 0.0f;
float accelerationY = 0.0f;
float accelerationZ = 0.0f;

float gyroscopeX = 0.0f;
float gyroscopeY = 0.0f;
float gyroscopeZ = 0.0f;

float linearMagnitude = 0.0f;
float angularMagnitude = 0.0f;

// Frequency and repetition internal state
float previousAngularVelocity = 0.0f;
unsigned long lastDirectionChangeMs = 0;

bool directionChangeStarted = false;

// --------------------------------------------------
// Initialization
// --------------------------------------------------

bool initializeAccelerometer() {
  if (!mpu.begin()) {
    return false;
  }

  /*
   * These settings match the provided working starter code.
   */
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  delay(100);

  accelerometerRunning = false;
  resetAnalysis();

  return true;
}

// --------------------------------------------------
// Start and stop
// --------------------------------------------------

void start_acc(SportType sport) {
  if (accelerometerRunning) {
    return;
  }

  if (sport == SPORT_UNKNOWN) {
    return;
  }

  currentSport = sport;

  resetAnalysis();

  accelerometerRunning = true;

  lastSampleTimeUs = micros();
}

void stop_acc() {
  if (!accelerometerRunning) {
    return;
  }

  accelerometerRunning = false;
}

bool isAccelerometerRunning() {
  return accelerometerRunning;
}

// --------------------------------------------------
// Sport selection
// --------------------------------------------------

SportType getCurrentSport() {
  return currentSport;
}

SportType parseSportType(const String &name) {
  if (name == "arm_rainbows") {
    return SPORT_ARM_RAINBOWS;
  }

  if (name == "arnold_press") {
    return SPORT_ARNOLD_PRESS;
  }

  if (name == "biceps_curls") {
    return SPORT_BICEPS_CURLS;
  }

  return SPORT_UNKNOWN;
}

const __FlashStringHelper *getSportName(SportType sport) {
  switch (sport) {
    case SPORT_ARM_RAINBOWS:
      return F("Arm rainbows");

    case SPORT_ARNOLD_PRESS:
      return F("Arnold press");

    case SPORT_BICEPS_CURLS:
      return F("Biceps curls");

    default:
      return F("Unknown");
  }
}

// --------------------------------------------------
// Reset functions
// --------------------------------------------------

void resetAnalysis() {
  accelerationX = 0.0f;
  accelerationY = 0.0f;
  accelerationZ = 0.0f;

  gyroscopeX = 0.0f;
  gyroscopeY = 0.0f;
  gyroscopeZ = 0.0f;

  linearMagnitude = 0.0f;
  angularMagnitude = 0.0f;

  previousAngularVelocity = 0.0f;
  lastDirectionChangeMs = 0;
  directionChangeStarted = false;

  movementValid = false;

  /*
   * These variables are defined in the main file but reset
   * as part of the complete analysis reset.
   */
  repetitionCount = 0;
  repetitionFrequency = 0.0f;

  angularPosition = 0.0f;
  currentROM = 0.0f;
  maximumROM = 0.0f;

  minimumAngle = 0.0f;
  maximumAngle = 0.0f;

  lastSampleTimeUs = micros();
}

void resetRepetitionCount() {
  repetitionCount = 0;
}

// --------------------------------------------------
// Sensor update
// --------------------------------------------------

void updateSensorData() {
  if (!accelerometerRunning) {
    return;
  }

  sensors_event_t accelerationEvent;
  sensors_event_t gyroEvent;
  sensors_event_t temperatureEvent;

  /*
   * Read accelerometer, gyroscope, and temperature data
   * using the Adafruit MPU6050 library.
   */
  mpu.getEvent(
    &accelerationEvent,
    &gyroEvent,
    &temperatureEvent
  );

  unsigned long currentTimeUs = micros();

  float deltaTime =
    (currentTimeUs - lastSampleTimeUs) / 1000000.0f;

  lastSampleTimeUs = currentTimeUs;

  /*
   * Ignore invalid or excessively long sample intervals.
   */
  if (deltaTime <= 0.0f || deltaTime > 0.1f) {
    return;
  }

  accelerationX = accelerationEvent.acceleration.x;
  accelerationY = accelerationEvent.acceleration.y;
  accelerationZ = accelerationEvent.acceleration.z;

  gyroscopeX = gyroEvent.gyro.x;
  gyroscopeY = gyroEvent.gyro.y;
  gyroscopeZ = gyroEvent.gyro.z;

  updateMagnitude();

  /*
   * Select the gyroscope axis that best represents the movement.
   *
   * This currently uses Z. Depending on the wristband orientation,
   * you may need to use gyroscopeX or gyroscopeY instead.
   */
  float selectedAngularVelocity = gyroscopeZ;

  /*
   * Convert rad/s to degrees/s for output.
   */
  angularMagnitude =
    fabsf(selectedAngularVelocity) * 180.0f / PI;

  /*
   * Integrate angular velocity to estimate angular position.
   *
   * This is an estimate and will drift over time.
   */
  angularPosition += selectedAngularVelocity * deltaTime;

  float angleDegrees =
    angularPosition * 180.0f / PI;

  /*
   * Track the minimum and maximum angle.
   */
  if (angleDegrees < minimumAngle) {
    minimumAngle = angleDegrees;
  }

  if (angleDegrees > maximumAngle) {
    maximumAngle = angleDegrees;
  }

  currentROM = maximumAngle - minimumAngle;

  if (currentROM > maximumROM) {
    maximumROM = currentROM;
  }

  updateFrequencyAndRepetitionCount();
}

// --------------------------------------------------
// Acceleration magnitude
// --------------------------------------------------

void updateMagnitude() {
  float totalAcceleration = sqrtf(
    accelerationX * accelerationX +
    accelerationY * accelerationY +
    accelerationZ * accelerationZ
  );

  /*
   * At rest, total acceleration is approximately 9.81 m/s².
   * Removing gravity gives an approximate linear movement intensity.
   */
  float dynamicAcceleration =
    fabsf(totalAcceleration - GRAVITY);

  /*
   * Low-pass filter.
   */
  const float filterAlpha = 0.2f;

  linearMagnitude =
    filterAlpha * dynamicAcceleration +
    (1.0f - filterAlpha) * linearMagnitude;
}

// --------------------------------------------------
// Frequency estimation
// --------------------------------------------------

void updateFrequencyAndRepetitionCount() {
  const float selectedAngularVelocity = gyroscopeZ;
  const unsigned long now = millis();

  bool positiveToNegative =
    previousAngularVelocity > GYRO_THRESHOLD_RAD_S &&
    selectedAngularVelocity <= 0.0f;

  bool negativeToPositive =
    previousAngularVelocity < -GYRO_THRESHOLD_RAD_S &&
    selectedAngularVelocity >= 0.0f;

  bool directionChanged =
    positiveToNegative || negativeToPositive;

  if (directionChanged) {
    if (lastDirectionChangeMs != 0) {
      unsigned long halfCycleTime =
        now - lastDirectionChangeMs;

      /*
       * Reject noise and implausibly slow movements.
       */
      if (halfCycleTime >= 100 &&
          halfCycleTime <= 5000) {

        float fullCycleSeconds =
          (2.0f * halfCycleTime) / 1000.0f;

        if (fullCycleSeconds > 0.0f) {
          repetitionFrequency =
            1.0f / fullCycleSeconds;
        }

        /*
         * Count one complete repetition after two direction changes.
         */
        if (directionChangeStarted) {
          repetitionCount++;
          directionChangeStarted = false;
        }
        else {
          directionChangeStarted = true;
        }
      }
    }
    else {
      /*
       * The first valid direction change starts the cycle.
       */
      directionChangeStarted = true;
    }

    lastDirectionChangeMs = now;
  }

  /*
   * This assignment must occur after all transition tests.
   */
  previousAngularVelocity = selectedAngularVelocity;
}


// --------------------------------------------------
// Sport analysis
// --------------------------------------------------

/*
 * Checks the current ROM and frequency against the given limits.
 */
static bool isWithinLimits(
  float minROM, float maxROM,
  float minFrequency, float maxFrequency
) {
  bool validROM =
    currentROM >= minROM &&
    currentROM <= maxROM;

  bool validFrequency =
    repetitionFrequency >= minFrequency &&
    repetitionFrequency <= maxFrequency;

  return validROM && validFrequency;
}

void analyze(SportType sport) {
  switch (sport) {
    case SPORT_ARM_RAINBOWS:
      analyzeArmRainbows();
      break;

    case SPORT_ARNOLD_PRESS:
      analyzeArnoldPress();
      break;

    case SPORT_BICEPS_CURLS:
      analyzeBicepsCurls();
      break;

    default:
      movementValid = false;
      break;
  }
}

void analyzeArmRainbows() {
  movementValid = isWithinLimits(
    MIN_ARM_RAINBOW_ROM, MAX_ARM_RAINBOW_ROM,
    MIN_ARM_RAINBOW_FREQUENCY, MAX_ARM_RAINBOW_FREQUENCY
  );
}

void analyzeArnoldPress() {
  movementValid = isWithinLimits(
    MIN_ARNOLD_PRESS_ROM, MAX_ARNOLD_PRESS_ROM,
    MIN_ARNOLD_PRESS_FREQUENCY, MAX_ARNOLD_PRESS_FREQUENCY
  );
}

void analyzeBicepsCurls() {
  movementValid = isWithinLimits(
    MIN_BICEPS_CURL_ROM, MAX_BICEPS_CURL_ROM,
    MIN_BICEPS_CURL_FREQUENCY, MAX_BICEPS_CURL_FREQUENCY
  );
}

bool isMovementValid() {
  return movementValid;
}

// --------------------------------------------------
// Getter functions
// --------------------------------------------------

float getLinearMagnitude() {
  return linearMagnitude;
}

float getAngularMagnitude() {
  return angularMagnitude;
}

float getAccelerationX() {
  return accelerationX;
}

float getAccelerationY() {
  return accelerationY;
}

float getAccelerationZ() {
  return accelerationZ;
}

float getGyroscopeX() {
  return gyroscopeX;
}

float getGyroscopeY() {
  return gyroscopeY;
}

float getGyroscopeZ() {
  return gyroscopeZ;
}