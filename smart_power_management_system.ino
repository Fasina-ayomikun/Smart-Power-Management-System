



#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <PZEM004Tv30.h>
#include <DHT.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#define TFT_CS 5
#define TFT_DC 4
#define TFT_RST 33

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define DHTPIN 13
#define DHTTYPE DHT11

#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#endif

float temp;
float humi;
DHT dht(DHTPIN, DHTTYPE);
int buzzerPin = 2;
int batteryPin = 32;
int liveRelay = 25;
int neutralRelay = 26;
int fanRelay = 14;
int socketsRelay = 15;
int displayRelay = 27;
int lightRelay = 22;
int tempThreshold = 27;

float batVoltage;
float inputBatVoltage;
float outputBatVoltage;
float resistor1 = 4660.00;
float resistor2 = 998.00;
int batPercentage;

int minPercent = 10;
int percentRange1 = 70;
int percentRange2 = 50;
int percentRange3 = 30;
int percentRange4 = 20;
int switchToGrid = 0;
PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);

const char *ssid = "RavaBot";
const char *password = "1122334455";

float powerFactor = 34;
float power = 65;
float energy = 64;
float frequency = 32;
float voltage = 54;
float current = 12;
float temperature = 34;
int fanStatus = 1;
int socketsStatus = 0;
int lightStatus = 0;
int displayStatus = 0;
int timeCount = 0;
int switched= 0;
AsyncWebServer server(80);
void setup() {
  dht.begin();
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  pinMode(batteryPin, INPUT);
  pinMode(liveRelay, OUTPUT);
  pinMode(neutralRelay, OUTPUT);
  pinMode(fanRelay, OUTPUT);
  pinMode(socketsRelay, OUTPUT);
  pinMode(displayRelay, OUTPUT);
  pinMode(lightRelay, OUTPUT);

  digitalWrite(liveRelay, LOW);
  digitalWrite(neutralRelay, LOW);


  tft.initR(INITR_144GREENTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  tft.drawRect(0, 0, 128, 128, ST77XX_CYAN);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 40);
  tft.println("RAVABOT");

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(35, 70);
  tft.println("Built By");
  
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(25, 85);
  tft.println("Team Lautech'25");

  tft.drawRect(20, 100, 88, 10, ST77XX_GREEN);
  tft.fillRect(20, 100, 40, 10, ST77XX_GREEN);
  delay(5000);
  handleServerData();
}



void loop() {
  timeCount++;
  handleBatLogic();

  if (switchToGrid) {
    digitalWrite(liveRelay, HIGH);
    digitalWrite(neutralRelay, HIGH);
  
  } else {
    digitalWrite(liveRelay, LOW);
    digitalWrite(neutralRelay, LOW);
   
  }
  pzemCode();
  dhtCode();

  if (timeCount >= 60) {
    timeCount = 0;
  }
  delay(100);
}

void handleServerData() {

  // Mount SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  // Create hotspot
  WiFi.softAP(ssid, password);
  // Serve static files
  server.serveStatic("/", SPIFFS, "/");

  // Default route - index.html
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Dashboard route - dashboard.html
  server.on("/dashboard", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/dashboard.html", "text/html");
  });

  server.on("/getData", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"powerFactor\":" + String(powerFactor) + ",";
    json += "\"power\":" + String(power) + ",";
    json += "\"energy\":" + String(energy) + ",";
    json += "\"batteryPercent\":" + String(batPercentage) + ",";
    json += "\"frequency\":" + String(frequency) + ",";
    json += "\"voltage\":" + String(voltage) + ",";
    json += "\"current\":" + String(current) + ",";
    json += "\"temperature\":" + String(temperature) + ",";
    json += "\"fanStatus\":" + String(fanStatus) + ",";
    json += "\"socketsStatus\":" + String(socketsStatus) + ",";
    json += "\"switch2grid\":" + String(switchToGrid) + ",";
    json += "\"lightStatus\":" + String(lightStatus) + ",";
    json += "\"displayStatus\":" + String(displayStatus);
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/getSettingsData", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"minBattery\":" + String(minPercent) + ",";
    json += "\"tempThreshold\":" + String(tempThreshold) + ",";
    json += "\"batRange1\":" + String(percentRange1) + ",";
    json += "\"batRange2\":" + String(percentRange2) + ",";
    json += "\"batRange3\":" + String(percentRange3) + ",";
    json += "\"batRange4\":" + String(percentRange4);
    json += "}";

    request->send(200, "application/json", json);
  });



  // Endpoint to receive data from dashboard to Arduino
  server.on("/setData", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("tempThreshold", true))
      tempThreshold = request->getParam("tempThreshold", true)->value().toFloat();
    if (request->hasParam("switch2grid", true))
      switchToGrid = request->getParam("switch2grid", true)->value().toInt();

    if (request->hasParam("minBattery", true))
      minPercent = request->getParam("minBattery", true)->value().toInt();

    if (request->hasParam("batRange1", true))
      percentRange1 = request->getParam("batRange1", true)->value().toFloat();
    if (request->hasParam("batRange2", true))
      percentRange2 = request->getParam("batRange2", true)->value().toFloat();
    if (request->hasParam("batRange3", true))
      percentRange3 = request->getParam("batRange3", true)->value().toFloat();
    if (request->hasParam("batRange4", true))
      percentRange4 = request->getParam("batRange4", true)->value().toFloat();

    Serial.println("Config updated from dashboard ");
    request->send(200, "text/plain", "OK");
  });

  server.begin();

  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(5, 10);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print("SSID: ");
  tft.println(ssid);

  tft.setCursor(5, 25);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print("Password: ");
  tft.println(password);
  delay(5000);
  return;
}
void handleBatLogic() {
  // Read raw ADC value
   batVoltage = analogRead(batteryPin);

  // Convert ADC to divider output voltage (ESP32 ADC is 12-bit, 0–4095)
   outputBatVoltage = (batVoltage * 3.24) / 4095.0;

  // Scale back to actual battery voltage
  inputBatVoltage = 1.28 + (outputBatVoltage * ((resistor1 + resistor2) / resistor2));

  batPercentage = map(inputBatVoltage * 100, 1080, 1270, 0, 10000) / 100;

  if (batPercentage > 100) batPercentage = 100;
  if (batPercentage < 0) batPercentage = 0;


  if (batPercentage <= percentRange1) {
    digitalWrite(fanRelay, LOW);
    fanStatus = 0;
  } else {
    digitalWrite(fanRelay, HIGH);
    fanStatus = 1;
  }

  if (batPercentage <= percentRange2) {
    digitalWrite(displayRelay, LOW);
    displayStatus = 0;
  } else {
    digitalWrite(displayRelay, HIGH);
    displayStatus = 1;
  }

  if (batPercentage <= percentRange3) {
    digitalWrite(lightRelay, LOW);
    lightStatus = 0;
  } else {
    digitalWrite(lightRelay, HIGH);
    lightStatus = 1;
  }

  if (batPercentage <= percentRange4) {
    digitalWrite(socketsRelay, LOW);
    socketsStatus = 0;
  } else {
    digitalWrite(socketsRelay, HIGH);
    socketsStatus = 1;
  }

  if (batPercentage < minPercent) {


    for (int i = 0; i < 3; i++) {
      digitalWrite(buzzerPin, HIGH);
      delay(50);
      digitalWrite(buzzerPin, LOW);
      delay(50);
    }
  }
 

  if(timeCount >=40 && timeCount <60){
       tft.fillScreen(ST77XX_BLACK);
    

      // Fan
      tft.setCursor(5, 10);
      tft.setTextColor(ST77XX_CYAN);
      tft.setTextSize(1);
      tft.print("Fan Status: ");
      tft.println(fanStatus ? "ON":"OFF");

      // Display
      tft.setCursor(5, 25);
      tft.setTextColor(ST77XX_GREEN);
      tft.setTextSize(1);
      tft.print("Display Status: ");
      tft.println(displayStatus ? "ON":"OFF");

      // Sockets
      tft.setCursor(5, 40);
      tft.setTextColor(ST77XX_YELLOW);
    
      tft.print("Sockets Status: ");
      tft.println(socketsStatus ? "ON":"OFF");
      // Central light
      tft.setCursor(5, 55);
      tft.setTextColor(ST77XX_CYAN);
     
      tft.print("Light Status: ");
      tft.println(lightStatus ? "ON":"OFF");
  }
  return;
}
void pzemCode() {
  // Read the data from the sensor
  voltage = pzem.voltage() ? pzem.voltage() : 0;
  current = pzem.current() ? pzem.current() : 0;
  power = pzem.power() ? pzem.power() : 0;
  energy = pzem.energy() ? pzem.energy() : 0;
  frequency = pzem.frequency() ? pzem.frequency() : 0;
  powerFactor = pzem.pf() ? pzem.pf() : 0;

  // Check if the data is valid
  if (isnan(voltage)) {
    Serial.println("Error reading voltage");

  } else if (isnan(current)) {
    Serial.println("Error reading current");
  } else if (isnan(power)) {
    Serial.println("Error reading power");
  } else if (isnan(energy)) {
    Serial.println("Error reading energy");
  } else if (isnan(frequency)) {
    Serial.println("Error reading frequency");
  } else if (isnan(powerFactor)) {
    Serial.println("Error reading power factor");
  } else {


    if (timeCount >= 0 && timeCount < 20) {
      tft.fillScreen(ST77XX_BLACK);

    // Battery Percentage
    tft.setCursor(5, 10);
    tft.setTextColor(ST77XX_CYAN);
    tft.setTextSize(1);
    tft.print("Battery (%): ");
    tft.print(batPercentage, 1);
    tft.println("%");
    
    // Battery Voltage
    tft.setCursor(5, 25);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(1);
    tft.print("Battery (V): ");
    tft.print(inputBatVoltage, 1);
    tft.println("V");
      // Voltage
      tft.setCursor(5, 40);
      tft.setTextColor(ST77XX_CYAN);
      tft.setTextSize(1);
      tft.print("Voltage: ");
      tft.print(voltage, 1);
      tft.println(" V");

      // Current
      tft.setCursor(5, 55);
      tft.setTextColor(ST77XX_GREEN);
      tft.setTextSize(1);
      tft.print("Current: ");
      tft.print(current, 2);
      tft.println(" A");

      // Power
      tft.setCursor(5, 70);
      tft.setTextColor(ST77XX_YELLOW);
      tft.print("Power: ");
      tft.print(power, 1);
      tft.println(" W");

      // Energy
      tft.setCursor(5, 85);
      tft.setTextColor(ST77XX_CYAN);
      tft.print("Energy: ");
      tft.print(energy, 3);
      tft.println(" kWh");

      // Frequency
      tft.setCursor(5, 100);
      tft.setTextColor(ST77XX_YELLOW);
      tft.print("Freq: ");
      tft.print(frequency, 1);
      tft.println(" Hz");

      // Power Factor
      tft.setCursor(5, 115);
      tft.setTextColor(ST77XX_WHITE);
      tft.print("PF: ");
      tft.print(powerFactor, 2);
    }
  }
  return;
}

void dhtCode() {
  temp = dht.readTemperature();
    humi = dht.readHumidity();

  if (timeCount >= 20 && timeCount < 40) {

    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(5, 10);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1);
    tft.print("Temp Lvl: ");
    tft.print(temp);
    tft.println("deg C");

     tft.setCursor(5, 25);
    tft.setTextColor(ST77XX_CYAN);
    tft.setTextSize(1);
    tft.print("Humidity Lvl: ");
    tft.print(humi);
    tft.println("%");
    
  static bool fanOn = false;             // remember last state
  const int onThr  = tempThreshold + 2;  // turn ON above this
  const int offThr = tempThreshold - 2;  // turn OFF below this
  if (!fanOn && temp > onThr) {
    fanOn = true;
    fanStatus= 1;
    digitalWrite(fanRelay, HIGH);
  } else if (fanOn && temp < offThr) {
    fanOn = false;
    fanStatus=0;
    digitalWrite(fanRelay, LOW);
  }

 
  }
}
