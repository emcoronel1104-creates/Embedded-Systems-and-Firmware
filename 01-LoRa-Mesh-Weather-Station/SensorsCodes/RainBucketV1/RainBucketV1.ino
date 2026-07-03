int tipCount = 0;
int lastState = HIGH;

void setup() {
  Serial.begin(9600);
  
  // FIXED: Changed from INPUT_PULLUP to INPUT
  pinMode(2, INPUT); 
  
  Serial.println("Tip counter ready");
}

void loop() {
  int currentState = digitalRead(2);
  
  if (currentState == LOW && lastState == HIGH) {
    tipCount++;
    Serial.print("Tips: ");
    Serial.println(tipCount);
    delay(50);  // Simple hardware/software debounce blend
  }
  
  lastState = currentState;
}