
int acs1 = 34;

float vpcount = 3.3 / 4095;

float sense = 0.1;
float samplesum = 0;
long lasttime = 0;
float samplecount = 0;

float mean = 0.0;
float value = 0.0;

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  lcd.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  //float val = analogRead(acs1);

  //float cur_cal = val * vpcount

  if (millis() > lasttime + 1) {
    samplesum += sq(analogRead(acs1));  // -sq512);
    samplecount++;

    lasttime = millis();
  }
  if (samplecount == 1000) {

    mean = samplesum / samplecount;
    value = sqrt(mean);

    Serial.println(value);
    value = value - 1925.0;
    samplesum = 0;
    samplecount = 0;

    float cur_cal = value * vpcount;

    if (cur_cal > 3.3) cur_cal = 3.3;
    if (cur_cal < 0) cur_cal = 0;

    cur_cal /= sense;


  Serial.println("Val: " + String(cur_cal));
  lcd.setCursor(0, 0);
  lcd.print(cur_cal);
  delay(1000);
  }
}
