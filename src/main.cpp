#include <Arduino.h>

// put function declarations here:

const int led = D10;

void setup() {
  // put your setup code here, to run once:
  pinMode(led, OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
   
  digitalWrite(led, HIGH);
  delay(1000);
  digitalWrite(led, LOW);
  delay(1000);


  uint32_t Vbatt = 0;
  for(int i = 0; i < 16; i++) {
    Vbatt = Vbatt + analogReadMilliVolts(D0); // ADC with correction   
  }
  float Vbattf = 2 * Vbatt / 16 / 1000.0;     // attenuation ratio 1/2, mV --> V
  Serial.println(Vbattf, 3);
  delay(1000);
}