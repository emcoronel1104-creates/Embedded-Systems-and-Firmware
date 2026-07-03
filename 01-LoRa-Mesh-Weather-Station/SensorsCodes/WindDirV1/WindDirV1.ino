// Define the analog pin connected to the wind vane voltage divider
const byte WIND_VANE_PIN = A3;

// Struct to store the direction lookup table mapping
struct WindLookup {
  int targetAdc;
  char* directionStr;
};

// Lookup table compiled using the datasheet's resistance values,
// a 10k ohm pull-up resistor, and a 5V supply.
// Order is sorted by ADC value from lowest to highest for easier matching.
// Lookup table compiled using the datasheet's resistance values,
// a 10k ohm pull-up resistor, and a 5V supply.
// CORRECTED & SORTED by ADC value from lowest to highest.
const WindLookup windTable[] = {
  { 66,  "112.5 (ESE)" }, // 688 ohms
  { 84,  "67.5 (ENE)" },  // 891 ohms
  { 93,  "90.0 (E)" },    // 1k ohms
  { 126, "157.5 (SSE)" }, // 1.41k ohms
  { 185, "135.0 (SE)" },  // 2.2k ohms
  { 244, "202.5 (SSW)" }, // 3.14k ohms
  { 287, "180.0 (S)" },   // 3.9k ohms
  { 406, "22.5 (NNE)" },  // 6.57k ohms
  { 461, "45.0 (NE)" },   // 8.2k ohms
  { 600, "247.5 (WSW)" }, // 14.12k ohms
  { 631, "225.0 (SW)" },  // 16k ohms
  { 713, "337.5 (NNW)" }, // 21.88k ohms
  { 785, "0.0 (N)" },     // 33k ohms
  { 829, "292.5 (WNW)" }, // 42.12k ohms
  { 887, "315.0 (NW)" },  // 64.9k ohms
  { 946, "270.0 (W)" }    // 120k ohms
};
const int TABLE_SIZE = sizeof(windTable) / sizeof(windTable[0]);

// Function to convert raw ADC value to a human-readable direction string
const char* getWindDirection(int adcValue) {
  // If reading is near 1023, the circuit is open (vane disconnected or parsing error)
  if (adcValue > 990) return "Disconnected/Unknown";

  // Find the closest matching entry in our lookup table
  int closestIndex = 0;
  int minDifference = abs(adcValue - windTable[0].targetAdc);

  for (int i = 1; i < TABLE_SIZE; i++) {
    int difference = abs(adcValue - windTable[i].targetAdc);
    if (difference < minDifference) {
      minDifference = difference;
      closestIndex = i;
    }
  }

  return windTable[closestIndex].directionStr;
}

void setup() {
  Serial.begin(115200);
  Serial.println("--- Weather Meter Wind Vane Test ---");
}

void loop() {
  // Take an average of multiple analog samples to filter out electrical noise
  long adcSum = 0;
  for (int i = 0; i < 10; i++) {
    adcSum += analogRead(WIND_VANE_PIN);
    delay(10);
  }
  int averageAdc = adcSum / 10;

  // Get the parsed direction string
  const char* direction = getWindDirection(averageAdc);

  // Print results to the Serial Monitor
  Serial.print("Raw ADC: ");
  Serial.print(averageAdc);
  Serial.print(" | Detected Direction: ");
  Serial.println(direction);

  delay(1000); // Update output once per second
}
