#include <SoftwareSerial.h>

unsigned char dataBuffer[4] = {};
int distanceValue;

// Define your chosen Rx pin. Tx is disabled (-1) to save pin 5!
const int rxPin = 5; // Connect to Sensor's TX
const int txPin = -1; 

// Initialize SoftwareSerial
SoftwareSerial mySerial(rxPin, txPin);

void setup() {
  // Hardware serial for your computer Serial Monitor
  Serial.begin(9600); 
  
  // Software serial for your sensor
  mySerial.begin(9600); 
  
  delay(500); 
  Serial.println("System Initialized. Reading Sensor...");
}

void loop() {
  // CRITICAL FIX: Check 'mySerial' instead of 'Serial'
  if (mySerial.available() >= 4) {
    
    // Check if the next byte is the required header (0xFF)
    if (mySerial.read() == 0xFF) {
      dataBuffer[0] = 0xFF;
      dataBuffer[1] = mySerial.read(); // High byte of distance
      dataBuffer[2] = mySerial.read(); // Low byte of distance
      dataBuffer[3] = mySerial.read(); // Checksum byte
      
      // Calculate and verify Checksum to filter out bad data packets
      byte calculatedSum = (dataBuffer[0] + dataBuffer[1] + dataBuffer[2]) & 0xFF;
      
      if (calculatedSum == dataBuffer[3]) {
        // Combine the High and Low bytes to calculate distance in millimeters
        distanceValue = (dataBuffer[1] << 8) + dataBuffer[2]; 
        
        // Output reading if it falls within the sensor's physical limits
        if (distanceValue >= 280) { 
          // Print to the computer Serial Monitor
          Serial.print("Distance: ");
          Serial.print(distanceValue / 10.0, 1); // Convert mm to cm
          Serial.println(" cm");
        } else {
          Serial.println("Target too close (< 28cm Blind Zone)");
        }
      }
    }
  }
  
  // A tiny delay to keep the serial stream smooth
  delay(30); 
}