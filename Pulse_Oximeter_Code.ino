#include <DFRobot_MAX30102.h>

DFRobot_MAX30102 particleSensor;


//Priyanshu this is the global heart rate variable. Often, if the person pushes the sensor too hard, or if external lighting hits the sensor it sometimes jumps to a high heartrate
//So, if you were planning on implementing the heart attack detector, you should make it so your heart rate is above a certain number multiple times, or it stays above that number for a while
//so that it doesn't accidently have any false positives. 
int32_t global_heart_rate = 0; 

void setup()
{
  Serial.begin(115200);

  while (!particleSensor.begin()) {
    Serial.println("MAX30102 was not found");
    delay(1000);
  }

  particleSensor.sensorConfiguration(/*ledBrightness=*/60, /*sampleAverage=*/SAMPLEAVG_4, \
                                  /*ledMode=*/MODE_MULTILED, /*sampleRate=*/SAMPLERATE_400, \
                                  /*pulseWidth=*/PULSEWIDTH_411, /*adcRange=*/ADCRANGE_16384);
}

void loop()
{
  int32_t blood_oxygen_percentage;
  int8_t blood_oxygen_valid;
  int32_t heart_rate;
  int8_t heart_rate_valid;

  particleSensor.heartrateAndOxygenSaturation(&blood_oxygen_percentage, &blood_oxygen_valid, &heart_rate, &heart_rate_valid);

  if (heart_rate_valid == 1 && blood_oxygen_valid == 1) {
    
    global_heart_rate = heart_rate; 

    Serial.print("Heart Rate: ");
    Serial.print(global_heart_rate); 
    Serial.print(" BPM | SpO2: ");
    Serial.print(blood_oxygen_percentage);
    Serial.println("%");
  } else {
    Serial.println("Calculating... Keep your finger perfectly still.");
  }

  delay(100); 
}
