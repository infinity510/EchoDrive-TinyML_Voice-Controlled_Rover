const int micPin = A0;
const int buttonPin = 2;
const int ledPin = 3;
const int resetpin = 4;
const int onboardLED = 13; 

// 200 samples * 5ms interval = 1000ms (1 second capture window)
const int numSamples = 200; 
int features[numSamples];

int ambientNoiseAverage = 0;
int threshold = 0;
const int thresholdMargin = 15; 

void setup() {
  Serial.begin(115200); 
  
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(resetpin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(onboardLED, OUTPUT);
  
  digitalWrite(ledPin, LOW); 
  digitalWrite(onboardLED, LOW);
  
  delay(1000); 
  calibrateEnvironment();
}

void calibrateEnvironment() {
  digitalWrite(onboardLED, HIGH);
  
  long sumOfReadings = 0;
  const int calibrationPasses = 10000;
  
  for (int i = 0; i < calibrationPasses; i++) {
    sumOfReadings += analogRead(micPin);
    delay(2); 
  }
  
  ambientNoiseAverage = sumOfReadings / calibrationPasses;
  threshold = ambientNoiseAverage + thresholdMargin;
  
  digitalWrite(onboardLED, LOW);
  
  Serial.print("\n calliberation done threshold = ");
  Serial.print(threshold);
  Serial.println();
}

void loop() {
  digitalWrite(ledPin, HIGH);
  
  while (digitalRead(buttonPin) == HIGH) {
    if (digitalRead(resetpin) == LOW) {
      digitalWrite(ledPin, LOW);
      calibrateEnvironment();
      delay(500); 
      digitalWrite(ledPin, HIGH);
    }
  }
  
  digitalWrite(ledPin, LOW);
  
  // 1. CAPTURE PHASE & AVERAGE CALCULATION
  long captureSum = 0;
  Serial.println("speakup");
  delay(100);
  
  for (int i = 0; i < numSamples; i++) {
    features[i] = analogRead(micPin) - threshold;
    captureSum += features[i];
    delay(5); 
  }
  
  int currentCaptureAverage = captureSum / numSamples;
  
  // 2. TEXT FORMATTING & TRANSMISSION
  Serial.print("threshold = ");
  Serial.println(threshold);
  
  Serial.print("average reading = ");
  Serial.println(currentCaptureAverage);
  
  // Print exactly 100 values sequentially downwards for Python to parse
  int printCount = 200; 
  for (int i = 0; i < printCount; i++) {
    // CRITICAL FIX: Serial.println ensures each value is on its own line
    Serial.println(features[i]); 
    delay(2); 
  }
  
  Serial.println(" \n readings successfully taken press again to go to next reading");
  
  // 3. DEBOUNCE
  delay(500); 
}