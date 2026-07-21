// ==========================================
// 1. PREPROCESSOR DIRECTIVES & HEADERS
// ==========================================
#include <EloquentTinyML.h>
#include <eloquent_tinyml/tensorflow.h>
#include "model_data.h" // Ensure the newly generated 800-input hex array is in your sketch folder

// ==========================================
// 2. TENSORFLOW LITE MEMORY ALLOCATION
// ==========================================
#define NUMBER_OF_INPUTS 800
#define NUMBER_OF_OUTPUTS 3

// Expanded from 16KB to 32KB to accommodate the larger 800-element dense layers
#define TENSOR_ARENA_SIZE 32768

Eloquent::TinyML::TensorFlow::TensorFlow<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;

// ==========================================
// 3. HARDWARE PIN DEFINITIONS (ESP32)
// ==========================================
const int micPin = 34;
const int buttonPin = 32;
const int statusLedPin = 13;

// Command Visualization LEDs
const int frontLedPin = 25;   // Class 0
const int reverseLedPin = 26; // Class 1
const int noiseLedPin = 27;   // Class 2

// ==========================================
// 4. GLOBAL VARIABLES
// ==========================================
float input_tensor[NUMBER_OF_INPUTS];
int threshold = 0;
const int thresholdMargin = 15;

// Persists across loop() calls so "every 5th trigger" actually works
static unsigned long triggerCount = 0;

// ==========================================
// 5. CALIBRATION ROUTINE
// ==========================================
void calibrateEnvironment() {
  long sumOfReadings = 0;

  for (int i = 0; i < 500; i++) {
    sumOfReadings += analogRead(micPin);
    delay(2);
  }

  threshold = (sumOfReadings / 500) + thresholdMargin;
  Serial.print("Diagnostic: Environmental calibration complete. Noise floor threshold set to: ");
  Serial.println(threshold);
}

// ==========================================
// 6. MICROCONTROLLER INITIALIZATION
// ==========================================
void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(statusLedPin, OUTPUT);
  pinMode(frontLedPin, OUTPUT);
  pinMode(reverseLedPin, OUTPUT);
  pinMode(noiseLedPin, OUTPUT);
  pinMode(micPin, INPUT);
  // Ensure all diagnostic LEDs are extinguished on boot
  digitalWrite(statusLedPin, LOW);
  digitalWrite(frontLedPin, LOW);
  digitalWrite(reverseLedPin, LOW);
  digitalWrite(noiseLedPin, LOW);

  // Bind the compiled C++ array to the Eloquent object -- ONCE, here only
  ml.begin(acoustic_nn_model);

  if (!ml.isOk()) {
    Serial.print("Model init failed: ");
    Serial.println(ml.getErrorMessage());
    while (true) delay(1000);
  }

  calibrateEnvironment();
}

// ==========================================
// 7. MAIN EXECUTION LOOP
// ==========================================
void loop() {
  if (triggerCount % 5 == 0) {
    calibrateEnvironment();
  }

  // Await physical user trigger
  while (digitalRead(buttonPin) == HIGH) {
    delay(10);
  }
  triggerCount++;

  digitalWrite(statusLedPin, HIGH);
  Serial.println("Start speaking");

  // 50ms buffer to match the exact training cadence
  delay(50);

  // Phase 1: Data Acquisition & Normalization
  for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
    // 1. Capture the raw integer and subtract the noise floor
    float raw_val = (float)(analogRead(micPin) - threshold);

    // 2. Normalize by dividing by 4095.0 to match the Python Keras training constraints
    input_tensor[i] = raw_val / 4095.0;

    // 3. Maintain precise 1.0-second recording window (800 * 1.25ms)
    delayMicroseconds(1250);
  }

  digitalWrite(statusLedPin, LOW);

  // Phase 2: Neural Network Inference
  float probabilities[NUMBER_OF_OUTPUTS];
  ml.predict(input_tensor, probabilities);

  if (!ml.isOk()) {
    Serial.print("Inference failed: ");
    Serial.println(ml.getErrorMessage());
    return;
  }

  Serial.printf("Raw probs: %.4f, %.4f, %.4f\n",
                probabilities[0], probabilities[1], probabilities[2]);

  // Phase 3: Data Extraction
  int predicted_class = ml.probaToClass(probabilities);
  float confidence = probabilities[predicted_class];

  Serial.print("Inference Result -> Class: ");
  Serial.print(predicted_class);
  Serial.print(" | True Confidence Score: ");
  Serial.println(confidence);

  // Phase 4: Hardware Actuation (0.75 Threshold)
  if (predicted_class == 0 && confidence > 0.75) {
    Serial.println("Actuation: FRONT command recognized. Illuminating Front LED.");
    digitalWrite(frontLedPin, HIGH);
    delay(1000);
    digitalWrite(frontLedPin, LOW);
  }
  else if (predicted_class == 1 && confidence > 0.75) {
    Serial.println("Actuation: REVERSE command recognized. Illuminating Reverse LED.");
    digitalWrite(reverseLedPin, HIGH);
    delay(1000);
    digitalWrite(reverseLedPin, LOW);
  }
  else if (predicted_class == 2 && confidence > 0.75) {
    Serial.println("Actuation: NOISE recognized. Illuminating Noise LED.");
    digitalWrite(noiseLedPin, HIGH);
    delay(1000);
    digitalWrite(noiseLedPin, LOW);
  }
  else {
    Serial.println("Actuation: Confidence below 0.75 threshold. Blinking Status LED to indicate failure.");
    for (int blink = 0; blink < 3; blink++) {
      digitalWrite(statusLedPin, HIGH);
      delay(150);
      digitalWrite(statusLedPin, LOW);
      delay(150);
    }
  }

  // Prevent immediate re-triggering and buffer overflow
  delay(500);
}
