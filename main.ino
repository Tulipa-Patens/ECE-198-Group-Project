#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "accelerometer.h"

// --------------------------------------------------
// Global MPU6050 object
// --------------------------------------------------

Adafruit_MPU6050 mpu;

// --------------------------------------------------
// Main-loop timing
// --------------------------------------------------

const unsigned long SAMPLE_INTERVAL_US = 10000;  // 100 Hz
const unsigned long OUTPUT_INTERVAL_MS = 500;

unsigned long lastSampleTimeUs = 0;
unsigned long lastOutputTimeMs = 0;

// --------------------------------------------------
// Variables intentionally kept in the main file
// --------------------------------------------------

// Repetition data
unsigned long repetitionCount = 0;
float repetitionFrequency = 0.0f;

// ROM tracking
float angularPosition = 0.0f;
float currentROM = 0.0f;
float maximumROM = 0.0f;

float minimumAngle = 0.0f;
float maximumAngle = 0.0f;

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {
  Serial.begin(115200);

  /*
   * Wait up to three seconds for the Serial Monitor.
   */
  unsigned long serialStartTime = millis();

  while (!Serial && millis() - serialStartTime < 3000) {
    delay(10);
  }

  Wire.begin();

  /*
   * The serial output is read by another program, so no text is
   * printed here. If the sensor is not found, the board stays idle.
   */
  if (!initializeAccelerometer()) {
    while (true) {
      delay(100);
    }
  }
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {
  processSerialCommands();


  if (!isAccelerometerRunning()) {
    delay(10); //remove when other programs are added to the loop; they take up time as well
    return;
  }

  unsigned long nowUs = micros();

  if (nowUs - lastSampleTimeUs >= SAMPLE_INTERVAL_US) {
    lastSampleTimeUs = nowUs;

    updateSensorData();
  }

  unsigned long nowMs = millis();

  if (nowMs - lastOutputTimeMs >= OUTPUT_INTERVAL_MS) {
    lastOutputTimeMs = nowMs;

    analyze(getCurrentSport());
    printResults();
  }
}

// --------------------------------------------------
// Serial commands
// --------------------------------------------------

/*
 * Commands:
 *   acc_start <sport>  - start detection for a sport
 *                        (arm_rainbows, arnold_press, biceps_curls)
 *   acc_stop           - stop detection
 *   acc_status         - print current values
 *   acc_reset          - reset all analysis values
 *   acc_reset_reps     - reset only repetition count
 *
 * Unknown commands are ignored silently.
 */
void processSerialCommands() {
  if (!Serial.available()) {
    return;
  }

  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toLowerCase();

  if (command.startsWith("acc_start")) {
    String sportName = command.substring(9);
    sportName.trim();

    start_acc(parseSportType(sportName));
  }
  else if (command == "acc_stop") {
    stop_acc();
  }
  else if (command == "acc_status") {
    printResults();
  }
  else if (command == "acc_reset") {
    resetAnalysis();
  }
  else if (command == "acc_reset_reps") {
    resetRepetitionCount();
  }
}

// --------------------------------------------------
// Results
// --------------------------------------------------

void printResults() {
  Serial.print(F("Sport: "));
  Serial.print(getSportName(getCurrentSport()));

  Serial.print(F(" | Repetitions: "));
  Serial.print(repetitionCount);

  Serial.print(F(" | Frequency: "));
  Serial.print(repetitionFrequency, 2);
  Serial.print(F(" Hz"));

  Serial.print(F(" | ROM: "));
  Serial.print(maximumROM, 1);
  Serial.println(F(" deg"));
}
