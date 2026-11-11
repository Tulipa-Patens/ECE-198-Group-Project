#include <Adafruit_MPU6050.h> 
#include <Adafruit_Sensor.h> //Adafruit Unified Sensor
#include <Wire.h> //Adafruit Bus IO

Adafruit_MPU6050 mpu;

// ---------- Filtering ----------
float gravityMagnitude = 9.80665f;
float filteredMotion = 0.0f;
float previousMotion = 0.0f;

// Low-pass filter coefficient for estimating gravity.
// Smaller values produce more smoothing.
const float gravityAlpha = 0.05f;

// Smoothing coefficient for the motion signal.
const float signalAlpha = 0.25f;

// ---------- Peak detection ----------
bool signalRising = false;
float signalPeak = 0.0f;
float signalTrough = 0.0f;

unsigned long lastPeakTime = 0;
unsigned long previousPeakTime = 0;

const unsigned long minimumRepTime = 300; 
// 300 ms prevents detecting more than approximately 200 repetitions/minute.

const unsigned long maximumRepTime = 3000;
// If no new peak is detected within this time, the frequency is reset.

float repetitionFrequencyHz = 0.0f;
float repetitionsPerMinute = 0.0f;

float repetitionMagnitudeG = 0.0f;
float currentMotionMagnitudeG = 0.0f;

// ---------- Threshold ----------
float motionThreshold = 0.20f;

// Set this to true while the user is moving.
// It can be used to perform automatic threshold calibration.
bool calibrationMode = false;



void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  
  
  Serial.println("");
  delay(100);

}

void loop() {
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  unsigned long now = millis();

  /*
   * ---------------------------------------------------------
   * 1. Calculate accelerometer and gyroscope magnitudes
   * ---------------------------------------------------------
   */

  // Accelerometer readings are in m/s^2.
  float accelerationMagnitude =
      sqrt(
        a.acceleration.x * a.acceleration.x +
        a.acceleration.y * a.acceleration.y +
        a.acceleration.z * a.acceleration.z
      );

  // Gyroscope readings from Adafruit MPU6050 are in rad/s.
  float gyroMagnitudeRad =
      sqrt(
        g.gyro.x * g.gyro.x +
        g.gyro.y * g.gyro.y +
        g.gyro.z * g.gyro.z
      );

  // Convert acceleration to g.
  float accelerationMagnitudeG = accelerationMagnitude / 9.80665f;

  /*
   * ---------------------------------------------------------
   * 2. Estimate and remove static gravity
   * ---------------------------------------------------------
   *
   * When the wrist is stationary, the accelerometer magnitude
   * is approximately 1 g. The low-pass filter estimates that
   * slowly changing value.
   */

  gravityMagnitude =
      (1.0f - gravityAlpha) * gravityMagnitude +
      gravityAlpha * accelerationMagnitudeG;

  // Dynamic acceleration caused by exercise.
  float dynamicAccelerationG =
      fabs(accelerationMagnitudeG - gravityMagnitude);

  /*
   * ---------------------------------------------------------
   * 3. Combine acceleration and gyro movement
   * ---------------------------------------------------------
   *
   * Acceleration is measured in g, while angular velocity is
   * measured in rad/s. Therefore, the gyro value is scaled
   * before being combined with acceleration.
   *
   * This coefficient should be experimentally adjusted.
   */

  const float gyroContributionScale = 0.08f;

  float gyroContribution =
      gyroMagnitudeRad * gyroContributionScale;

  float rawMotionSignal =
      dynamicAccelerationG + gyroContribution;

  /*
   * ---------------------------------------------------------
   * 4. Smooth the combined motion signal
   * ---------------------------------------------------------
   */

  filteredMotion =
      (1.0f - signalAlpha) * filteredMotion +
      signalAlpha * rawMotionSignal;

  currentMotionMagnitudeG = filteredMotion;

  /*
   * ---------------------------------------------------------
   * 5. Optional threshold calibration
   * ---------------------------------------------------------
   *
   * During calibration, the threshold can be adjusted based
   * on the current motion level. For normal operation, use a
   * fixed threshold first.
   */

  float threshold = motionThreshold;

  if (calibrationMode) {
    threshold = max(0.12f, filteredMotion * 0.55f);
  }

  /*
   * ---------------------------------------------------------
   * 6. Peak detection
   * ---------------------------------------------------------
   *
   * A repetition is registered when the signal rises above
   * the threshold, reaches a peak, and then begins falling.
   */

  if (filteredMotion > signalPeak) {
    signalPeak = filteredMotion;
  }

  if (signalPeak > threshold) {
    signalRising = true;
  }

  // Detect a peak when the signal starts falling.
  if (signalRising &&
      filteredMotion < previousMotion &&
      filteredMotion < signalPeak * 0.90f) {

    unsigned long timeSinceLastPeak = now - lastPeakTime;

    if (timeSinceLastPeak >= minimumRepTime) {

      /*
       * Magnitude is the height of the detected motion peak.
       * This value is approximately expressed in g-equivalent
       * motion, because gyro data was scaled and combined with
       * acceleration data.
       */
      repetitionMagnitudeG = signalPeak;

      /*
       * -----------------------------------------------------
       * 7. Calculate frequency
       * -----------------------------------------------------
       */

      if (lastPeakTime != 0) {
        unsigned long period = now - lastPeakTime;

        if (period > 0 && period <= maximumRepTime) {
          repetitionFrequencyHz = 1000.0f / period;
          repetitionsPerMinute = repetitionFrequencyHz * 60.0f;
        }
      }

      previousPeakTime = lastPeakTime;
      lastPeakTime = now;

      // Reset peak detection for the next repetition.
      signalPeak = 0.0f;
      signalRising = false;

      /*
       * Output one result for every detected repetition.
       */
      Serial.print("Repetition detected | Frequency: ");
      Serial.print(repetitionFrequencyHz, 2);
      Serial.print(" Hz (");
      Serial.print(repetitionsPerMinute, 1);
      Serial.print(" repetitions/min) | Magnitude: ");
      Serial.print(repetitionMagnitudeG, 2);
      Serial.println(" g-equivalent");
    }
  }

  previousMotion = filteredMotion;

  /*
   * ---------------------------------------------------------
   * 8. Reset frequency if the user stops exercising
   * ---------------------------------------------------------
   */

  if (lastPeakTime != 0 &&
      now - lastPeakTime > maximumRepTime) {

    repetitionFrequencyHz = 0.0f;
    repetitionsPerMinute = 0.0f;
    signalPeak = 0.0f;
    signalRising = false;
  }

  /*
   * ---------------------------------------------------------
   * 9. Optional continuous diagnostic output
   * ---------------------------------------------------------
   *
   * Uncomment this section when tuning the threshold.
   */

  /*
  Serial.print("Motion: ");
  Serial.print(filteredMotion, 3);
  Serial.print(" | Threshold: ");
  Serial.print(threshold, 3);
  Serial.print(" | Frequency: ");
  Serial.print(repetitionsPerMinute, 1);
  Serial.print(" rpm | Magnitude: ");
  Serial.println(repetitionMagnitudeG, 3);
  */

  // The MPU6050 is commonly sampled at approximately 50 Hz.
  delay(20);
}
