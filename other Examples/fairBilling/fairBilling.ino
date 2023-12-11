
//Libraries for ESP32
#include <Wire.h>
#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#endif

#include <EEPROM.h>
#define EEPROM_SIZE 4

//Sensor Variables
float current = 0;
float voltage = 0;
float power = 0;

//PIN DEFINITIONS
int load = 32;
int current_pin = 34;

//Current and Voltage Libraries
#include "ACS712.h"
ACS712 ACS(current_pin, 3.3, 4095, 66);
#include "EmonLib.h"
int voltage_pin = 35;
#define VOLT_CAL 592
EnergyMonitor v;

//LCD Libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// Set the LCD address to 0x27 for a 16 chars and 2 line display
//LiquidCrystal_I2C lcd(0x27, 20, 4);
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Replace with your network credentials
const char* ssid = "PIEEIPPIE";
const char* password = "Xjok23dl";
String serverName = "https://imimapp.onrender.com/";  //http://127.0.0.1:5000 for local dev or set 0.0.0.0 in flask env
const char* api_key = "xdol";
int cloud = 0;
int flag = 1;
String response = "";
String state = "";
String unitb = "";    // is the unit bought
int unitl = 0;  //unitl is the unit left in the meter
float ect = 0;
float ecm = 0; 
int val = 0;
int store = 0;

//Send to Cloud details
unsigned long previous_time = 0;
const unsigned long interval_to_send = 5000;
unsigned long past_time = 0;
unsigned long current_time = 0;

void setup() {
  //PIN SETUP
  pinMode(load, OUTPUT);
  pinMode(current_pin, INPUT);
  pinMode(voltage_pin, INPUT);

  Serial.begin(115200);
  lcd.begin();
  lcd.backlight();

  //Setup Voltage & current
  ACS.autoMidPoint();
  ACS.getMidPoint();
  ACS.getNoisemV();
  v.voltage(voltage_pin, VOLT_CAL, 1.7);

  //SYSTEM INIT
  long t = millis();
  while (millis() - t > 5000) {
    lcd.setCursor(0, 0);
    lcd.print("Welcome");
    lcd.setCursor(0, 1);
    lcd.print("Smart Utility");
    lcd.setCursor(0, 2);
    lcd.print("Monitoring System");
    lcd.setCursor(0, 3);
    lcd.print("Initializing...");
    delay(3000);
    lcd.clear();
  }

  //WIFI INIT
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting");
  int i = 1;
  while (WiFi.status() != WL_CONNECTED && i > 0) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(i - 1, 1);
    lcd.print(".");
    i++;
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  //SYSTEM INIT
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Initialization");
  lcd.setCursor(0, 2);
  lcd.print("Successful");
  delay(3000);

  //Get the unit left from the EEPROM
  EEPROM.begin(EEPROM_SIZE);
  unitl = eeprom_read(0);
}

void loop() {
  //Is there Unit available on the Meter?
  if (unitl <= 0) {
    digitalWrite(load, 0);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Meter Is Out Of Unit");
    lcd.setCursor(0, 2);
    lcd.print("Please Recharge");
    unitb = send_to_cloud("get_unitb");
    if ((unitb[0] - '0') > 0) {
      unitl += unitb.toInt();
      send_to_cloud("del_unitb");
    }
    return;
  }

  //Is the Meter turned On?
  state = send_to_cloud("get_state");
  digitalWrite(load, (state[0] - '0'));
  if ((state[0] - '0') == 0) {
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("Meter is Off");
    lcd.setCursor(4, 1);
    lcd.print("Turn Meter On");
    return;
  }

  //Calculate I, V, P
  current = int(read_current());
  voltage = int(read_voltage());
  float new_voltage = constrain(map(voltage, 30, 1410, 0, 230), 227, 230);
  power = current * new_voltage;

  //Calculate KWh and update unitl
  past_time = current_time;
  current_time = millis();
  ecm = ecm + power * ((current_time - past_time) / 2.628e+9);
  ecm = ecm / 1000.0;                                                //Convert to KWh
  ect = ect + power * ((current_time - past_time) / 3600000.0);  //covert to watt / hr (with the extra 000 converting millis to hours)
  ect = ect / 1000.0;         //convert to KWh
  val = unitl;
  unitl -= (ect * 50.0) / 1000.0; //using a tarrif of #50 per KWh and divide by 1000 to mimimize the effect of the tariff
  val = abs(val - unitl);
 store += val;
  Serial.print(store);

  //PRINT INFO ON LCD
 lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Voltage: ");
  lcd.print(new_voltage, 2);
  lcd.print("V");

  lcd.setCursor(0, 1);
  lcd.print("Current: ");
  lcd.print(current, 2);
  lcd.print("A");

  lcd.setCursor(0, 2);
  lcd.print("Power: ");
  lcd.print((power / 1000.00), 2);
  lcd.print("KW");

  lcd.setCursor(0, 3);
  lcd.print("Unit Left: ");
  lcd.print(unitl);
  //Send Data to cloud every interval_to_send secs.
  cloud = 1;
  if (cloud) {
    if (millis() - previous_time >= interval_to_send) {
      String request = "update/key=" + String(api_key) + "/c1=" + String(int(power / 1000.00))
                       + "/v=" + String(int(new_voltage)) + "/ect=" + String(store) + "/ecm=" + String(store) + "/unitl=" + String(int(unitl));
      send_to_cloud(request);
      previous_time = millis();
    }
  }

  //Get Unit Bought
  unitb = send_to_cloud("get_unitb");
  if ((unitb[0] - '0') > 0) {
    unitl += unitb.toInt();
    send_to_cloud("del_unitb");
  }

  //write the current unitl to eeprom
  eeprom_write(0, unitl);
}

float read_current() {
  float average = 0;
  for (int i = 0; i < 100; i++) {
    average += ACS.mA_AC();
  }
  float curr = (average / 100.0);
  curr = curr * 1e-3;  //convert from mA to A
  return curr;
}

float read_voltage() {
  v.calcVI(20, 2000);
  float volt = v.Vrms;
  delay(10);
  return volt;
}

void eeprom_write(int address, int value) {
  byte* value_ptr = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++) {
    EEPROM.write(address + i, *(value_ptr + i));
  }
  EEPROM.commit();
}

int eeprom_read(int address) {
  int value = 0;
  byte* value_ptr = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++) {
    *(value_ptr + i) = EEPROM.read(address + i);
  }
  return value;
}

String send_to_cloud(String data) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String httpRequestData = data;
    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);

    http.begin((serverName + httpRequestData).c_str());
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Send HTTP GET request
    int httpResponseCode = http.GET();
    delay(1100);  //This should give time for the server to process the request.

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code:");
      Serial.println(httpResponseCode);
      if (data == "get_state") {
        response = http.getString();
        return response;
      }
      if (data == "get_unitb") {
        response = http.getString();
        return response;
      }
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}
