#include <Arduino.h>

const int potPins[4] = {32, 33, 34, 35};
const int switchPin = 25;

const int NUM_SAMPLES = 9;
const int OVERSAMPLE = 4;
  
const unsigned long DEBOUNCE_MS = 50;

int readFilteredADC(int pin){
  int samples[NUM_SAMPLES];
  for(int i=0;i<NUM_SAMPLES;i++){
    unsigned int sum = 0;
    for(int k=0;k<OVERSAMPLE;k++){
      sum += analogRead(pin);
      delayMicroseconds(50);
    }
    samples[i] = sum / OVERSAMPLE;
    delay(2);
  }
  for(int i=0;i<NUM_SAMPLES-1;i++){
    for(int j=i+1;j<NUM_SAMPLES;j++){
      if(samples[j] < samples[i]){
        int t = samples[i]; samples[i] = samples[j]; samples[j] = t;
      }
    }
  }
  return samples[NUM_SAMPLES/2];
}

void setup() {
  Serial.begin(115200);
  delay(100);

  for(int i=0;i<4;i++){
    pinMode(potPins[i], INPUT);
  }

  pinMode(switchPin, INPUT_PULLUP);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Serial.println("TX ready: reading pots on 32,33,34,35; switch on 25.");
}

void loop(){
  static unsigned long lastDebounceTime = 0;
  static int lastSwitchState = HIGH;
  static int stableSwitchState = HIGH;

  int values[4];
  for(int i=0;i<4;i++){
    values[i] = readFilteredADC(potPins[i]);
  }

  int rawSwitch = digitalRead(switchPin);
  if(rawSwitch != lastSwitchState){
    lastDebounceTime = millis();
    lastSwitchState = rawSwitch;
  }
  if((millis() - lastDebounceTime) > DEBOUNCE_MS){
    if(stableSwitchState != rawSwitch){
      stableSwitchState = rawSwitch;
    }
  }

  Serial.print(values[0]); Serial.print(',');
  Serial.print(values[1]); Serial.print(',');
  Serial.print(values[2]); Serial.print(',');
  Serial.print(values[3]); Serial.print(',');
  Serial.println(stableSwitchState == LOW ? "ON" : "OFF");

  delay(100);
}

