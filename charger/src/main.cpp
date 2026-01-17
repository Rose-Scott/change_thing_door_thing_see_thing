#include <Arduino.h>

// put function declarations here:

const int num_leds = 4;
const int charge_leds[num_leds] = {D7, D8, D9, D10};

const int batt_pin = A0;

const int plug_pin = D9;

void setup() {
  Serial.begin(115200);

  // put your setup code here, to run once:
  for (int i = 0; i < num_leds; i++) {
    pinMode(charge_leds[i], OUTPUT);
  }

  pinMode(batt_pin, INPUT); // voltage check pin

}

void loop() {
  // put your main code here, to run repeatedly:

  uint32_t Vbatt = 0;
  for(int i = 0; i < 16; i++) {
    Vbatt = Vbatt + analogReadMilliVolts(batt_pin); // ADC with correction
  }
  float Vbattf = 2 * Vbatt / 16 / 1000.0;     // attenuation ratio 1/2, mV --> V

  const float min_percent = 3.5;
  const float max_percent = 4.1;
  float percent = (Vbattf - min_percent) / (max_percent - min_percent) * 100;
  
  static uint32_t ms = millis();
  
  if (millis() - ms >= 1000) {
    Serial.printf("Percent: %.1f%\n", percent);
    ms = millis();
    
    for (int i = 0; i < num_leds; i++) {
      int threshold = 100 * ((i + 1.0) / num_leds);
      if (percent >= threshold) {
        digitalWrite(charge_leds[i], HIGH);
      } else {
        digitalWrite(charge_leds[i], LOW);
      }
    }
  }
}
