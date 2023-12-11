#include <HardwareSerial.h>

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

//Current and Voltage Libraries
#include "ACS712.h"
#include "EmonLib.h"

//Used for LCD Screen
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

//GSM VARIABLES
unsigned int temp = 0, i = 0, x = 0, k = 0;
char str[200], flag1 = 0, flag2 = 0;
String bal = ""; //2348179685579
String initSms = "AT+CMGS=\"+2348179685579\"\r\n";  //the number to send SMS
//ESP32 Serial2 for GSM Communication
HardwareSerial sim800(2);
#define baudrate 115200

//Sensor variables
float voltage;
float current_gen;
float current_dist;
float threshold = 0.5;
int theft = 0;
float power = 0.0;

//Pin Definitions
int current_dist_pin = 32;
int current_gen_pin = 34;
ACS712 ACS1(current_dist_pin, 3.3, 4095, 66);
ACS712 ACS2(current_gen_pin, 3.3, 4095, 66);
int voltage_pin = 33;
#define VOLT_CAL 230
EnergyMonitor volt;

//Define Relay
int relay_state = 1;
int relay_pin = 23;

void setup() {
  // put your setup code here, to run once:
  digitalWrite(relay_pin, 1);
  pinMode(relay_pin, OUTPUT);
  Serial.begin(115200);

  volt.voltage(voltage_pin, VOLT_CAL, 1.7);
  sim800.begin(baudrate, SERIAL_8N1, 16, 17);  //(RX, TX)

  lcd.begin();
  /* 
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  } 
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP()); */

  gsm_init();
}



void loop() {
  //Turn on relay
  if (relay_state) {
    digitalWrite(relay_pin, relay_state);
  }

  //Read Parameters
  voltage = read_voltage();
  current_gen = read_current(ACS2);
  current_dist = read_current(ACS1);
  float offset = current_dist - current_gen;
  voltage = constrain(voltage, 227, 230);
  power = voltage * current_dist;

  Serial.print("Current GEN: ");
  Serial.print(current_gen);
  Serial.print(" Current DIST: ");
  Serial.println(current_dist);

  //Display parameters and load consumption on screen
  lcd.setCursor(0, 0);
  lcd.print("Voltage: ");
  lcd.print(voltage);
  lcd.print("V");
  lcd.setCursor(0, 1);
  lcd.print("G-Current: ");
  lcd.print(current_gen);
  lcd.print("A");
  lcd.setCursor(0, 2);
  lcd.print("D-Current: ");
  lcd.print(current_dist);
  lcd.print("A");
  lcd.setCursor(0, 3);
  lcd.print("Power Cons: ");
  lcd.print(power);
  lcd.print("W");

  //Theft detecion query
  if ((current_gen > 1) && (current_gen - current_dist) > threshold) {
    theft = 1;
  } else {
    theft = 0;
  }

  //Send message in case of theft and trip off load
  if (theft) {
    relay_state = 0;
    digitalWrite(relay_pin, relay_state);
    sendSms("Theft has Occurred");
    delay(500);
  }
}


float read_voltage() {
  volt.calcVI(20, 2000);
  float v = volt.Vrms;
  delay(10);
  return v;
}

float read_current(ACS712 name) {
  float average = 0;
  for (int i = 0; i < 100; i++) {
    average += name.mA_AC();
  }
  float curr = (average / 100.0);
  curr = curr * 1e-3;  //convert from mA to A
  return curr;
}

//GSM INIT
void gsm_init() {  //Define a new serial for sim800
  lcd.clear();
  lcd.print("Finding GSM/GPRS");
  Serial.println("Finding GSM/GPRS..");
  boolean at_flag = 1;
  while (at_flag) {
    sim800.println("AT");
    while (sim800.available() > 0) {
      if (sim800.find("OK"))
        at_flag = 0;
    }
    delay(1000);
  }
  lcd.clear();
  lcd.print("Module Connected..");
  Serial.println("Module Connected..");
  delay(1000);
  lcd.clear();
  lcd.print("Disabling ECHO");
  Serial.print("Disabling ECHO");
  boolean echo_flag = 1;
  while (echo_flag) {
    sim800.println("ATE0");
    while (sim800.available() > 0) {
      if (sim800.find("OK"))
        echo_flag = 0;
    }
    delay(1000);
  }
  lcd.clear();
  lcd.print("Echo OFF");
  Serial.print("\tEcho OFF");
  delay(1000);
  lcd.clear();
  lcd.print("Finding Network..");
  Serial.println("\t Finding Network..");
  boolean net_flag = 1;
  while (net_flag) {
    sim800.println("AT+CPIN?");
    while (sim800.available() > 0) {
      if (sim800.find("+CPIN: READY"))
        net_flag = 0;
    }
    delay(1000);
  }
  lcd.clear();
  lcd.print("Network Found..");
  Serial.println("Network Found..");
  delay(1000);
  lcd.clear();
}

//SEND SMS
void sendSms(String text) {
  init_sms();
  Serial.println("Sending:");
  send_data(text);
  send_sms();
}

void init_sms() {
  sim800.println("AT+CSMP=17,167,0,0\r\n");
  delay(200);
  sim800.println("AT+CMGF=1\r\n");
  delay(200);
  sim800.println(initSms);
  delay(200);
}

void send_data(String message) {
  sim800.println(message);
  Serial.println(message);
  delay(200);
}

void send_sms() {
  sim800.write(26);
  message_sent();
}

void message_sent() {
  lcd.clear();
  lcd.print("Message Sent.");
  Serial.println("Message Sent.");
  delay(1000);
  //sim800.flush();
}
