#include <HardwareSerial.h>
#include <EEPROM.h>
#define EEPROM_SIZE 12

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

// Replace with your network credentials
const char* ssid = "EXPERIMENT";
const char* password = "Realvest";
String serverName = "https://boluapp.onrender.com/";
const char* api_key = "xdol";

//GSM VARIABLES
unsigned int temp = 0, i = 0, x = 0, k = 0;
char str[200], flag1 = 0, flag2 = 0;
String bal = "";
String initSms = "AT+CMGS=\"+2348109045164\"";  //the number to send SMS
String BalanceSms = "This is an SMS from Smart Energy Meter & your balance is ";

//ESP32 Serial2 for GSM Communication
HardwareSerial sim800(2);
#define baudrate 115200

//Sensor variables
float voltage;
float current_draw;
float c_draw_threshold = 5.0;

//CALCULATION VARIABLES
int balance = 25;
float unitPrice = 1.50f;
long lastPrint = millis();
long lastDtl = millis();
int overload = 0;
int balance_threshold = 20;
bool alert = true;
float power = 0;

int unitl = 25;  //unitl is the unit left in the meter
float ect = 0;
float ecm = 0;
int unit = 0;

//Pin Definitions
int current_draw_pin = 34;
int voltage_pin = 33;
#define VOLT_CAL 592
EnergyMonitor volt;
ACS712 ACS(current_draw_pin, 3.3, 4095, 66);

//Define Relay
int relay_state = 1;
int relay_pin = 23;

//Send to Cloud details
unsigned long previous_time = 0;
const unsigned long interval_to_send = 5000;
unsigned long past_time = 0;
unsigned long current_time = 0;
String response = "";


void setup() {
  // put your setup code here, to run once:
  EEPROM.begin(EEPROM_SIZE);
  pinMode(relay_pin, OUTPUT);
  Serial.begin(115200);
  volt.voltage(voltage_pin, VOLT_CAL, 1.7);
  sim800.begin(baudrate, SERIAL_8N1, 16, 17);  //(RX, TX)
  lcd.begin();

  gsm_init();
  
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
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  storeInfo();
  //reStoreInfo();
}



void loop() {
  //Read Meter State
  String m_state = send_to_cloud("get_state");
  int status = m_state[0] - '0';
  if (!status) {
    relay_state = 0;
    digitalWrite(relay_pin, relay_state);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Meter is Off");
    lcd.setCursor(0, 1);
    lcd.print("Turn it On");
    return;
  }
  //Read Parameters
  voltage = read_voltage();
  voltage = constrain(voltage, 227, 230);
  current_draw = read_current_draw();
  if (balance > 0 && (overload == 0)) {
    relay_state = 1;
    digitalWrite(relay_pin, relay_state);
    loadCalculation();
    storeInfo();
    printDetails();
    printBalance();
    checkLowBalance();
    checkSMS();
  } else {
    relay_state = 0;
    digitalWrite(relay_pin, relay_state);
    lcd.clear();
    lcd.print("Low Balance");
    lcd.setCursor(0, 1);
    lcd.print("Please Recharge");
    sendSms("Your Balance is Finished, Please recharge");
    delay(500);
  }
  //Overloading Test
  if (current_draw > c_draw_threshold) {
    overload = 1;
  } else {
    overload = 0;
  }
  //Send message in case of overload and trip off load
  if (overload) {
    relay_state = 0;
    digitalWrite(relay_pin, relay_state);
    sendSms("OVERLOADING HAS OCCURED");
    //See also receiving reply from sms to turn the relay back on.
  }
  //Send Data to cloud every interval_to_send secs.
  int cloud = 1;
  if (cloud) {
    if (millis() - previous_time >= interval_to_send) {
      String req = "update/key=" + String(api_key) + "/c1=" + String(int(current_draw))
                   + "/v=" + String(int(voltage));
      send_to_cloud(req);
      previous_time = millis();
    }
  }

  checkSMS();
  Serial.flush();
  sim800.flush();
}

//VOLTAGE AND CURRENT CONSUMPTION CALCULATION
float read_voltage() {
  volt.calcVI(20, 2000);
  float v = volt.Vrms;
  delay(10);
  return v;
}

float read_current_draw() {
  float average = 0;
  for (int i = 0; i < 100; i++) {
    average += ACS.mA_AC();
  }
  float curr = (average / 100.0);
  curr = curr * 1e-3;  //convert from mA to A
  return curr;
}

//PRINT INFO ON LCD
void printInfo() {
  lcd.clear();
  lcd.print("Balance:");
  lcd.print(balance);
  lcd.print("NGN");
  lcd.setCursor(0, 1);
  lcd.print("Unit left:");
  lcd.print(unitl);
  lcd.print(" KWtt");
  delay(100);
}
void printDetails() {
  lcd.clear();
  lcd.print("Current:");
  lcd.print(current_draw, 3);
  lcd.print("Amps");
  lcd.setCursor(0, 1);
  lcd.print("Power:");
  lcd.print(power);
  lcd.print(" Watt");
  delay(1000);
}

void printBalance() {
  if ((millis() - lastPrint) > 5000)  //print balance and usage every 5 seconds without using delay()
  {
    printInfo();
    delay(1000);
    lastPrint = millis();
  }
}


//PERFORM LOAD CALCULATION AND UPDATE BALANCE
void checkLowBalance() {
  if (balance < 20 && alert)  //notify customer about low balance
  {
    sendSms("Your Balance is Below 20NGN, Please recharge to avoid disconnection");
    alert = false;
    // cashbalance = 0;
  }
}

void storeInfo(){
  //  EEPROM[100]=unit;
  //  EEPROM[0]=balance;
  eeprom_write(0, unitl);
  eeprom_write(100, balance);
}

void reStoreInfo() {
  //  unit= EEPROM[100];
  //  balance= EEPROM[0];
  unitl = eeprom_read(0);
  balance = eeprom_read(100);
}

void loadCalculation() {
  //IMPORTED SOME CORRECTIONS
  power = voltage * current_draw;
  //load =(power/(10.0*36.0)); // instantanious load
  //unit+=load;                      //Cumulated load or power consumption
  //balance -=load*unitPrice;        // balance adjustment
  past_time = current_time;
  current_time = millis();
  ect += power * ((current_time - past_time) / 3600000.0);  //covert to watt / hr (with the extra 000 converting millis to hours)
  ect /= 1000.0;                                            //convert to KWh
  //unit += power/100.0; //this minimizes the effect of the power drawn
  balance -= ect * 50 / 1000.0;  //using a tarrif of #50 per KWh and divide by 1000 to mimimize the effect of the tariff
  unitl -= ect / 1000.0;
}

//CHECK SMS AND RECEIVE BALANCE VIA SMS
void checkSMS() {
  sim800.println("AT+CMGF=1"); // Configuring TEXT mode
  sim800.println("AT+CNMI=1,2,0,0,0");
  GPRSEvent();
  balanceCheck();
  if (temp == 1) {
    decode_message();
    send_confirmation_sms();
  }
}

void GPRSEvent() {
  while (sim800.available()) {
    char ch = (char)sim800.read();
    str[i++] = ch;
    if (ch == '*') {
      temp = 1;
      lcd.clear();
      lcd.print("Message Received");
      Serial.print("Message Received:\t");

      delay(500);
      break;
    }
  }
}
void decode_message() {
  x = 0, k = 0, temp = 0;
  while (str[x] != '#') {
    x++;
  }
  x++;
  bal = "";
  while (str[x] != '*') {
    bal += str[x];
    x++;
  }
  Serial.println(bal.toInt());
}

void send_confirmation_sms() {
  int recharge_amount = bal.toInt();
  balance += recharge_amount;
  lcd.clear();
  lcd.print("Energy Meter ");
  lcd.setCursor(0, 1);
  lcd.print("Recharged:");
  //FLAGS BEWARE
  alert = true;  // resets the flag for the system to
  //check for balance again
  //cashbalance = 1;// resets flag for running of the
  //whole major system in the loop
  lcd.print(recharge_amount);
  delay(2000);
  String text1 = "Energy meter recharged NGN:";
  text1 += recharge_amount;
  String text2 = text1 + ". Total Balance " + (String)balance;
  sendSms(text2);
  Serial.print("           SENT:");
  Serial.print(text1);
  Serial.println(text2);
  temp = 0;
  i = 0;
  x = 0;
  k = 0;
  delay(1000);
}

void balanceCheck() {
  x = 0, k = 0;
  String Stringcheck = "";
  while (x < i) {
    Stringcheck += str[x];
    x++;
  }
  if (Stringcheck.indexOf("Balance?") > 0) {
    sendSms("Total Balance " + (String)balance);
    for (int c = 0; c < i; c++) {
      str[c] = 'a';
    }
  }
}

//ALL ABOUT GSM INIT
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
  sim800.println("AT+CSMP=17,167,0,0");
  delay(200);
  sim800.println("AT+CMGF=1");
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

//ALL ABOUT SENDING TO CLOUD

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
      return String(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
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
