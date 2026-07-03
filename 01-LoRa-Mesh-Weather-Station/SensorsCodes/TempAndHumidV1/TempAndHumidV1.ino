#include <Wire.h>
#include "DFRobot_SHT20.h"

DFRobot_SHT20 sht20(&Wire, SHT20_I2C_ADDR);

void setup() {
  Serial.begin(9600);
  Wire.begin();
  
  // Wait for serial connection to settle
  delay(1000);
  
  Serial.println("SHT20 Sensor Test Starting...");
  
  // Initialize the sensor
  sht20.initSHT20();
  
  // Check if sensor is responding
  delay(100);
  sht20.checkSHT20();
  
  Serial.println("Setup complete. Reading sensor data...");
}

void loop() {
  // Read temperature and humidity
  float temp = sht20.readTemperature();
  float humidity = sht20.readHumidity();
  
  // Check for valid readings
  if (temp != ERROR_I2C_TIMEOUT && humidity != ERROR_I2C_TIMEOUT) {
    Serial.print("Temperature: ");
    Serial.print(temp, 2);
    Serial.print(" °C, ");
    Serial.print("Humidity: ");
    Serial.print(humidity, 2);
    Serial.println(" %RH");
  } else {
    Serial.println("Sensor reading error! Check connections.");
  }
  
  delay(1000); // Read every second
}
