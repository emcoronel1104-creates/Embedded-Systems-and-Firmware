#include <Wire.h>
#include "RTClib.h"
#include <SoftwareSerial.h>
#include "DFRobot_SHT20.h"

// --- RTC CONFIG ---
RTC_DS3231 rtc;

// --- SHT20 CONFIG ---
DFRobot_SHT20 sht20(&Wire, SHT20_I2C_ADDR);
float currentTemp = 0.0;
float currentHumidity = 0.0;
unsigned long lastSHTReadTime = 0;

// --- ULTRASONIC SENSOR CONFIG ---
unsigned char dataBuffer[4] = {};
int distanceValue = 0;
const int rxPin = 5; 
const int txPin = -1; 
SoftwareSerial mySerial(rxPin, txPin); 

// --- ANEMOMETER (WIND SPEED) CONFIG ---
const byte ANEMOMETER_PIN = 2; 
volatile unsigned long windPulseCount = 0;
const float WIND_CALIBRATION = 2.4; 
const unsigned long WIND_DEBOUNCE = 15; 
volatile unsigned long lastWindPulseTime = 0;

// --- RAIN GAUGE CONFIG ---
const byte RAIN_PIN = 3; 
volatile unsigned long rainTipCount = 0;
const unsigned long RAIN_DEBOUNCE = 50;
volatile unsigned long lastRainPulseTime = 0;

// --- WIND VANE CONFIG ---
const byte WIND_VANE_PIN = A3;
struct WindLookup {
  int targetAdc;
  const char* directionStr;
};

const WindLookup windTable[] = {
  { 66,  "112.5 (ESE)" }, { 84,  "67.5 (ENE)" },  { 93,  "90.0 (E)" },    { 126, "157.5 (SSE)" },
  { 185, "135.0 (SE)" },  { 244, "202.5 (SSW)" }, { 287, "180.0 (S)" },   { 406, "22.5 (NNE)" },
  { 461, "45.0 (NE)" },   { 600, "247.5 (WSW)" }, { 631, "225.0 (SW)" },  { 713, "337.5 (NNW)" },
  { 785, "0.0 (N)" },     { 829, "292.5 (WNW)" }, { 887, "315.0 (NW)" },  { 946, "270.0 (W)" }
};
const int TABLE_SIZE = sizeof(windTable) / sizeof(windTable[0]);
const char* windDir = "Unknown";

// --- REPORT TIMING ---
unsigned long lastReportTime = 0;
const unsigned long reportInterval = 5000; 

// --- INTERRUPT SERVICE ROUTINES (ISRs) ---
void countWindPulse() {
  unsigned long currentTime = millis();
  if (currentTime - lastWindPulseTime > WIND_DEBOUNCE) {
    windPulseCount++;
    lastWindPulseTime = currentTime;
  }
}

void countRainPulse() {
  unsigned long currentTime = millis();
  if (currentTime - lastRainPulseTime > RAIN_DEBOUNCE) {
    rainTipCount++;
    lastRainPulseTime = currentTime;
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(9600); 
  mySerial.begin(9600); 
  Wire.begin();
  
  pinMode(ANEMOMETER_PIN, INPUT_PULLUP); 
  pinMode(RAIN_PIN, INPUT_PULLUP); 
  pinMode(WIND_VANE_PIN, INPUT);         

  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), countWindPulse, FALLING);
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), countRainPulse, FALLING);

  if (!rtc.begin()) {
    Serial.println("Warning: RTC not found!");
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  sht20.initSHT20();
  delay(100);
  sht20.checkSHT20();
  
  Serial.println("Timestamp,Distance(cm),RainTips,WindSpeed(km/h),WindDirection,Temp(C),Humidity(%)");
  lastReportTime = millis();
}

// --- MAIN LOOP ---
void loop() {
  readUltrasonic();  
  readWindVane();    
  readSHT20();       
  sendCompressedReport();
  
  delay(30); 
}

// --- SENSOR FUNCTIONS ---

void readUltrasonic() {
  while (mySerial.available() > 0) {
    byte inByte = mySerial.read();
    
    dataBuffer[0] = dataBuffer[1];
    dataBuffer[1] = dataBuffer[2];
    dataBuffer[2] = dataBuffer[3];
    dataBuffer[3] = inByte;
    
    if (dataBuffer[0] == 0xFF) {
      byte calculatedSum = (dataBuffer[0] + dataBuffer[1] + dataBuffer[2]) & 0xFF;
      if (calculatedSum == dataBuffer[3]) {
        distanceValue = (dataBuffer[1] << 8) + dataBuffer[2]; 
      }
    }
  }
}

void readWindVane() {
  if (millis() - lastReportTime >= (reportInterval - 100)) { 
    long adcSum = 0;
    for (int i = 0; i < 10; i++) adcSum += analogRead(WIND_VANE_PIN);
    int avgWindAdc = adcSum / 10;
    
    if (avgWindAdc > 990) {
      windDir = "Disconnected";
    } else {
      int closestIndex = 0;
      int minDifference = abs(avgWindAdc - windTable[0].targetAdc);
      for (int i = 1; i < TABLE_SIZE; i++) {
        int difference = abs(avgWindAdc - windTable[i].targetAdc);
        if (difference < minDifference) {
          minDifference = difference;
          closestIndex = i;
        }
      }
      windDir = windTable[closestIndex].directionStr;
    }
  }
}

void readSHT20() {
  if (millis() - lastSHTReadTime >= 2500) {
    lastSHTReadTime = millis();
    float t = sht20.readTemperature();
    float h = sht20.readHumidity();
    if (t != ERROR_I2C_TIMEOUT && h != ERROR_I2C_TIMEOUT) {
      currentTemp = t;
      currentHumidity = h;
    }
  }
}

void sendCompressedReport() {
  if (millis() - lastReportTime >= reportInterval) {
    unsigned long timeElapsed = millis() - lastReportTime;
    lastReportTime = millis();
    
    noInterrupts();
    unsigned long windPulses = windPulseCount;
    unsigned long rainTips = rainTipCount;
    windPulseCount = 0; 
    rainTipCount = 0; 
    interrupts();
    
    float frequency = (float)windPulses / (timeElapsed / 1000.0);
    float windSpeedKmH = frequency * WIND_CALIBRATION;

    DateTime now = rtc.now();
    
    char distStr[10], windStr[10], tempStr[10], humStr[10];
    float finalDistance = (distanceValue >= 280) ? (distanceValue / 10.0) : 0.0;
    
    dtostrf(finalDistance, 4, 1, distStr);
    dtostrf(windSpeedKmH, 4, 1, windStr);
    dtostrf(currentTemp, 4, 2, tempStr);
    dtostrf(currentHumidity, 4, 2, humStr);

    char reportBuffer[160];
    snprintf(reportBuffer, sizeof(reportBuffer), 
             "%04d-%02d-%02d %02d:%02d:%02d, %s cm, %lu tips, %s km/h, %s, %s C, %s %%",
             now.year(), now.month(), now.day(), 
             now.hour(), now.minute(), now.second(),
             distStr, 
             rainTips, 
             windStr,
             windDir,
             tempStr,
             humStr);
             
    Serial.println(reportBuffer);
  }
}