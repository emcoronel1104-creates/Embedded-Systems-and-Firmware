// Pin definition for the anemometer (Nano v3 supports hardware interrupts on D2 and D3)
const byte ANEMOMETER_PIN = 2; 

// Variables for pulse counting and timing
volatile unsigned long pulseCount = 0;
unsigned long lastMillis = 0;

// Calibration factor from datasheet: 1 pulse per second (1Hz) = 2.4 km/h [cite: 22]
const float CALIBRATION_FACTOR = 2.4; 

// Debounce time in milliseconds to prevent mechanical switch bounce noise
const unsigned long DEBOUNCE_TIME = 15; 
volatile unsigned long lastPulseTime = 0;

// Interrupt Service Routine (ISR) called every time the sensor switch closes
void countPulse() {
  unsigned long currentTime = millis();
  // Simple software debounce
  if (currentTime - lastPulseTime > DEBOUNCE_TIME) {
    pulseCount++;
    lastPulseTime = currentTime;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configure the pin with internal pull-up resistor
  pinMode(ANEMOMETER_PIN, INPUT_PULLUP);
  
  // Attach interrupt: triggers when pin goes from HIGH to LOW (switch closes)
  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), countPulse, FALLING);
  
  Serial.println("--- Weather Meter Anemometer Test ---");
  lastMillis = millis();
}

void loop() {
  // Calculate and display wind speed every 3 seconds
  if (millis() - lastMillis >= 3000) {
    
    // Temporarily disable interrupts while reading and resetting the counter
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();
    
    unsigned long timeElapsed = millis() - lastMillis;
    lastMillis = millis();
    
    // Calculate Frequency (Hz = pulses per second)
    float frequency = (float)pulses / (timeElapsed / 1000.0);
    
    // Calculate wind speed
    float windSpeedKmH = frequency * CALIBRATION_FACTOR;
    float windSpeedMS = windSpeedKmH / 3.6; // Convert km/h to m/s
    
    // Output data to the Serial Monitor
    Serial.print("Pulses: ");
    Serial.print(pulses);
    Serial.print(" | Freq: ");
    Serial.print(frequency, 2);
    Serial.print(" Hz | Wind Speed: ");
    Serial.print(windSpeedKmH, 2);
    Serial.print(" km/h (");
    Serial.print(windSpeedMS, 2);
    Serial.println(" m/s)");
  }
}