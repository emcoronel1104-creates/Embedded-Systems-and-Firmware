/*
 * Complete Rain Gauge with DS3231 RTC
 * Supports Minute, Hourly, and Daily reporting intervals
 * Optimized for memory and performance
 */

#include <Wire.h>
#include <RTClib.h>

// ============================================
// USER CONFIGURATION - CHANGE THIS SECTION
// ============================================

// Select reporting interval: 1 = MINUTE, 2 = HOURLY, 3 = DAILY
#define REPORT_MODE 1  // <-- CHANGE THIS: 1=Minute, 2=Hourly, 3=Daily

// Rain gauge calibration (mm per tip)
#define MM_PER_TIP 0.2794
#define INCHES_PER_TIP 0.011

// Pin definitions
#define RAIN_PIN 2
#define LED_PIN 13

// Debounce delay (milliseconds)
#define DEBOUNCE_DELAY 50

// Status report interval (milliseconds) - set to 0 to disable
#define STATUS_INTERVAL 60000  // 1 minute (set to 0 to disable)

// ============================================
// GLOBAL VARIABLES
// ============================================

RTC_DS3231 rtc;

// Rain tracking
volatile unsigned long tipCount = 0;
volatile bool tipDetected = false;
float intervalRain = 0;      // Rain for current reporting interval
float dailyRain = 0;         // Rain today (for daily reset)

// Time tracking
int lastMinute = -1;
int lastHour = -1;
int lastDay = -1;

// Debouncing
int lastState = HIGH;
unsigned long lastDebounceTime = 0;

// Status reporting
unsigned long lastStatusTime = 0;

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(9600);
  printHeader();
  
  // Initialize pins
  pinMode(RAIN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  // Test LED
  digitalWrite(LED_PIN, HIGH);
  delay(300);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize RTC
  if (!initRTC()) {
    errorBlink();  // RTC not found - infinite blink
  }
  
  // Get initial time
  DateTime now = rtc.now();
  lastMinute = now.minute();
  lastHour = now.hour();
  lastDay = now.day();
  
  // Print system ready message
  printReady();
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  // Process rain tips
  processRainSensor();
  
  // Check for reporting intervals
  DateTime now = rtc.now();
  
  if (REPORT_MODE == 1) {
    checkMinuteRollover(now);
  } else if (REPORT_MODE == 3) {
    checkDailyRollover(now);
  } else {  // HOURLY (default)
    checkHourRollover(now);
  }
  
  // Periodic status report
  if (STATUS_INTERVAL > 0 && (millis() - lastStatusTime >= STATUS_INTERVAL)) {
    printStatus();
    lastStatusTime = millis();
  }
  
  delay(10);  // Small delay for stability
}

// ============================================
// RAIN SENSOR FUNCTIONS
// ============================================

void processRainSensor() {
  int currentState = digitalRead(RAIN_PIN);
  
  // Detect falling edge (HIGH to LOW)
  if (currentState == LOW && lastState == HIGH) {
    if ((millis() - lastDebounceTime) >= DEBOUNCE_DELAY) {
      // Valid tip detected
      tipCount++;
      intervalRain += MM_PER_TIP;
      dailyRain += MM_PER_TIP;
      
      // Visual feedback
      digitalWrite(LED_PIN, HIGH);
      delay(30);
      digitalWrite(LED_PIN, LOW);
      
      // Print quick feedback
      printTipFeedback();
      
      lastDebounceTime = millis();
    }
  }
  lastState = currentState;
}

// ============================================
// ROLLOVER CHECKS
// ============================================

void checkMinuteRollover(DateTime now) {
  if (now.minute() != lastMinute) {
    if (intervalRain > 0) {
      reportMinuteRainfall(now);
    }
    intervalRain = 0;
    lastMinute = now.minute();
  }
  
  // Also check daily rollover for daily total
  if (now.day() != lastDay) {
    if (dailyRain > 0) {
      reportDailyRainfall(now);
    }
    dailyRain = 0;
    lastDay = now.day();
  }
}

void checkHourRollover(DateTime now) {
  if (now.hour() != lastHour) {
    if (intervalRain > 0) {
      reportHourlyRainfall(now);
    }
    intervalRain = 0;
    lastHour = now.hour();
  }
  
  // Also check daily rollover
  if (now.day() != lastDay) {
    if (dailyRain > 0) {
      reportDailyRainfall(now);
    }
    dailyRain = 0;
    lastDay = now.day();
  }
}

void checkDailyRollover(DateTime now) {
  if (now.day() != lastDay) {
    if (intervalRain > 0 || dailyRain > 0) {
      reportDailyRainfall(now);
    }
    intervalRain = 0;
    dailyRain = 0;
    lastDay = now.day();
  }
}

// ============================================
// REPORTING FUNCTIONS
// ============================================

void reportMinuteRainfall(DateTime now) {
  Serial.println(F("\n╔════════════════════════════════════╗"));
  Serial.println(F("║      MINUTE RAINFALL REPORT       ║"));
  Serial.println(F("╚════════════════════════════════════╝"));
  
  printTimestamp(now);
  
  Serial.print(F("Rain in last minute: "));
  printRainfall(intervalRain);
  
  Serial.println(F("══════════════════════════════════════\n"));
}

void reportHourlyRainfall(DateTime now) {
  Serial.println(F("\n╔════════════════════════════════════╗"));
  Serial.println(F("║      HOURLY RAINFALL REPORT       ║"));
  Serial.println(F("╚════════════════════════════════════╝"));
  
  printTimestamp(now);
  
  int reportHour = (lastHour == 0) ? 24 : lastHour;
  Serial.print(F("Hour ending: "));
  Serial.print(reportHour);
  Serial.println(F(":00"));
  
  Serial.print(F("Rainfall: "));
  printRainfall(intervalRain);
  printIntensity(intervalRain);
  
  // CSV format for data logging
  printCSV("HOUR", now, reportHour, intervalRain);
  
  Serial.println(F("══════════════════════════════════════\n"));
}

void reportDailyRainfall(DateTime now) {
  Serial.println(F("\n╔════════════════════════════════════╗"));
  Serial.println(F("║       DAILY RAINFALL REPORT        ║"));
  Serial.println(F("╚════════════════════════════════════╝"));
  
  printTimestamp(now);
  
  Serial.print(F("Date: "));
  Serial.print(now.month());
  Serial.print(F("/"));
  Serial.print(lastDay);
  Serial.print(F("/"));
  Serial.println(now.year());
  
  Serial.print(F("Total rainfall: "));
  printRainfall(dailyRain);
  printDailyClassification(dailyRain);
  
  // CSV format
  printCSV("DAY", now, lastDay, dailyRain);
  
  Serial.println(F("══════════════════════════════════════\n"));
}

// ============================================
// UTILITY FUNCTIONS
// ============================================

void printHeader() {
  Serial.println(F("========================================"));
  Serial.print(F("   Rain Gauge with "));
  
  if (REPORT_MODE == 1) {
    Serial.println(F("MINUTE Reporting"));
  } else if (REPORT_MODE == 3) {
    Serial.println(F("DAILY Reporting"));
  } else {
    Serial.println(F("HOURLY Reporting"));
  }
  
  Serial.println(F("========================================\n"));
  
  Serial.println(F("Calibration:"));
  Serial.print(F("  Each tip = "));
  Serial.print(MM_PER_TIP, 4);
  Serial.print(F(" mm ("));
  Serial.print(INCHES_PER_TIP, 3);
  Serial.println(F(" inches)"));
  Serial.println();
}

void printReady() {
  Serial.println(F("=== SYSTEM READY ==="));
  Serial.print(F("Current time: "));
  printDateTime(rtc.now());
  Serial.println();
  Serial.println(F("Waiting for rain events...\n"));
}

void printTipFeedback() {
  Serial.print(F("💧 Tip #"));
  Serial.print(tipCount);
  Serial.print(F(" | "));
  
  if (REPORT_MODE == 1) {
    Serial.print(F("This minute: "));
  } else if (REPORT_MODE == 3) {
    Serial.print(F("Today: "));
  } else {
    Serial.print(F("This hour: "));
  }
  
  Serial.print(intervalRain, 1);
  Serial.print(F("mm | Daily: "));
  Serial.print(dailyRain, 1);
  Serial.println(F("mm"));
}

void printRainfall(float mm) {
  Serial.print(mm, 1);
  Serial.print(F(" mm ("));
  Serial.print(mm / MM_PER_TIP, 1);
  Serial.print(F(" tips)"));
  Serial.println();
}

void printIntensity(float mm) {
  Serial.print(F("Intensity: "));
  if (mm < 0.1) Serial.println(F("None"));
  else if (mm < 2.5) Serial.println(F("Light"));
  else if (mm < 10) Serial.println(F("Moderate"));
  else if (mm < 30) Serial.println(F("Heavy"));
  else Serial.println(F("Extreme"));
}

void printDailyClassification(float mm) {
  Serial.print(F("Classification: "));
  if (mm == 0) Serial.println(F("Dry Day"));
  else if (mm < 2.5) Serial.println(F("Very Light"));
  else if (mm < 10) Serial.println(F("Light Rain"));
  else if (mm < 30) Serial.println(F("Moderate Rain"));
  else if (mm < 60) Serial.println(F("Heavy Rain"));
  else Serial.println(F("Extreme Rain"));
}

void printCSV(const char* period, DateTime dt, int periodValue, float rainMM) {
  Serial.print(F("DATA,"));
  Serial.print(period);
  Serial.print(F(","));
  Serial.print(dt.year());
  Serial.print(F("-"));
  Serial.print(dt.month());
  Serial.print(F("-"));
  Serial.print(dt.day());
  Serial.print(F(","));
  Serial.print(periodValue);
  Serial.print(F(","));
  Serial.print(rainMM, 1);
  Serial.print(F(","));
  Serial.println(rainMM * INCHES_PER_TIP / MM_PER_TIP, 3);
}

void printStatus() {
  DateTime now = rtc.now();
  
  Serial.println(F("\n┌────────────────────────────────────┐"));
  Serial.println(F("│        CURRENT STATUS              │"));
  Serial.println(F("├────────────────────────────────────┤"));
  
  Serial.print(F("│ Time: "));
  printTimeCompact(now);
  Serial.println(F(" │"));
  
  Serial.print(F("│ "));
  if (REPORT_MODE == 1) {
    Serial.print(F("This minute: "));
  } else if (REPORT_MODE == 3) {
    Serial.print(F("Today: "));
  } else {
    Serial.print(F("This hour: "));
  }
  Serial.print(intervalRain, 1);
  Serial.print(F(" mm"));
  padSpaces(intervalRain, 1, 12);
  Serial.println(F("│"));
  
  Serial.print(F("│ Rain Today: "));
  Serial.print(dailyRain, 1);
  Serial.print(F(" mm"));
  padSpaces(dailyRain, 1, 15);
  Serial.println(F("│"));
  
  Serial.print(F("│ Total Tips: "));
  Serial.print(tipCount);
  padSpaces(tipCount, 0, 16);
  Serial.println(F("│"));
  
  Serial.print(F("│ Temperature: "));
  Serial.print(rtc.getTemperature(), 1);
  Serial.print(F("°C"));
  padSpaces(rtc.getTemperature(), 1, 14);
  Serial.println(F("│"));
  
  Serial.println(F("└────────────────────────────────────┘\n"));
}

void printTimestamp(DateTime dt) {
  Serial.print(F("Time: "));
  printDateTime(dt);
  Serial.println();
}

void printDateTime(DateTime dt) {
  Serial.print(dt.year());
  Serial.print(F("/"));
  Serial.print(dt.month());
  Serial.print(F("/"));
  Serial.print(dt.day());
  Serial.print(F(" "));
  Serial.print(dt.hour());
  Serial.print(F(":"));
  if (dt.minute() < 10) Serial.print(F("0"));
  Serial.print(dt.minute());
  Serial.print(F(":"));
  if (dt.second() < 10) Serial.print(F("0"));
  Serial.print(dt.second());
}

void printTimeCompact(DateTime dt) {
  if (dt.hour() < 10) Serial.print(F("0"));
  Serial.print(dt.hour());
  Serial.print(F(":"));
  if (dt.minute() < 10) Serial.print(F("0"));
  Serial.print(dt.minute());
  Serial.print(F(":"));
  if (dt.second() < 10) Serial.print(F("0"));
  Serial.print(dt.second());
}

void padSpaces(float value, int decimals, int width) {
  String str = String(value, decimals);
  int spaces = width - str.length();
  for (int i = 0; i < spaces; i++) Serial.print(F(" "));
}

void padSpaces(unsigned long value, int decimals, int width) {
  String str = String(value);
  int spaces = width - str.length();
  for (int i = 0; i < spaces; i++) Serial.print(F(" "));
}

// ============================================
// RTC FUNCTIONS
// ============================================

bool initRTC() {
  if (!rtc.begin()) {
    Serial.println(F("ERROR: DS3231 not found!"));
    Serial.println(F("Check wiring:"));
    Serial.println(F("  VCC → 5V"));
    Serial.println(F("  GND → GND"));
    Serial.println(F("  SDA → A4"));
    Serial.println(F("  SCL → A5"));
    return false;
  }
  
  // Set time (uncomment ONE of these lines)
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Compile time
  rtc.adjust(DateTime(2026, 6, 9, 14, 30, 0));  // Manual time
  
  Serial.println(F("DS3231 RTC initialized!"));
  return true;
}

void errorBlink() {
  while (1) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}

// ============================================
// SERIAL COMMANDS
// ============================================

void serialEvent() {
  if (!Serial.available()) return;
  
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toUpperCase();
  
  if (cmd == "STATUS" || cmd == "S") {
    printStatus();
  }
  else if (cmd == "RESET") {
    tipCount = 0;
    intervalRain = 0;
    dailyRain = 0;
    Serial.println(F("All counters reset!"));
  }
  else if (cmd == "TIME") {
    Serial.print(F("Current time: "));
    printDateTime(rtc.now());
    Serial.println();
  }
  else if (cmd == "HELP" || cmd == "H") {
    Serial.println(F("\nCommands:"));
    Serial.println(F("  STATUS/S - Show status"));
    Serial.println(F("  RESET    - Reset counters"));
    Serial.println(F("  TIME     - Show time"));
    Serial.println(F("  HELP/H   - This help\n"));
  }
}