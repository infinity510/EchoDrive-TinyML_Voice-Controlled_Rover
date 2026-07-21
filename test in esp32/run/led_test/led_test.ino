// ==========================================
// 1. PREPROCESSOR DIRECTIVES & HEADERS
// ==========================================
#include <EloquentTinyML.h>
#include <eloquent_tinyml/tensorflow.h> 
#include "model_data.h" 

// ==========================================
// 2. TENSORFLOW LITE MEMORY ALLOCATION
// ==========================================
#define NUMBER_OF_INPUTS 200
#define NUMBER_OF_OUTPUTS 3
#define TENSOR_ARENA_SIZE 16384 

Eloquent::TinyML::TensorFlow::TensorFlow<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;

// ==========================================
// 3. HARDWARE PIN DEFINITIONS (ESP32)
// ==========================================
const int micPin = 34; 
const int buttonPin = 32; 

// System status indicator (illuminates during 1-second recording window)
const int statusLedPin = 13; 

// Command Visualization LEDs
// Connect the positive (longer) leg of each LED to these pins, and the negative leg to GND via a 220-ohm resistor.
const int frontLedPin = 25;   // Class 0
const int reverseLedPin = 26; // Class 1
const int noiseLedPin = 27;   // Class 2

// ==========================================
// 4. GLOBAL VARIABLES
// ==========================================
float input_tensor[NUMBER_OF_INPUTS]; 
int threshold = 0;
const int thresholdMargin = 15;

// ==========================================
// 5. CALIBRATION ROUTINE
// ==========================================
void calibrateEnvironment() {
  long sumOfReadings = 0;
  Serial.println("calliberation started");
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
  
  analogReadResolution(10); 
  
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(statusLedPin, OUTPUT);
  pinMode(frontLedPin, OUTPUT);
  pinMode(reverseLedPin, OUTPUT);
  pinMode(noiseLedPin, OUTPUT);
  
  // Ensure all LEDs are extinguished on boot
  digitalWrite(statusLedPin, LOW);
  digitalWrite(frontLedPin, LOW);
  digitalWrite(reverseLedPin, LOW);
  digitalWrite(noiseLedPin, LOW);
  
  ml.begin(acoustic_nn_model);
  
  calibrateEnvironment();
}

// ==========================================
// 7. MAIN EXECUTION LOOP
// ==========================================
// ==========================================
// 7. MAIN EXECUTION LOOP (PATCHED)
// ==========================================
void loop() {
  // Await user trigger 
  while (digitalRead(buttonPin) == HIGH) {
    delay(10);
  }
  
  digitalWrite(statusLedPin, HIGH);
  
  // Phase 1: Data Acquisition with Voltage Correction
  // 1.515 multiplier artificially scales 3.3V ESP32 signals to match 5V UNO training data
  const float voltage_correction_factor = 1.515;
  
  for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
    int raw_val = analogRead(micPin) - threshold;
    input_tensor[i] = (float)(raw_val) * voltage_correction_factor;
    delay(5); 
  }
  
  digitalWrite(statusLedPin, LOW);
  
  // Phase 2: Neural Network Inference (Outputs raw logits)
  float probabilities[NUMBER_OF_OUTPUTS];
  ml.predict(input_tensor, probabilities);
  
  // Phase 3: Manual Softmax Conversion (Logits -> Probabilities)
  // 1. Find the maximum logit for numerical stability
  float max_logit = -10000.0;
  for(int i = 0; i < NUMBER_OF_OUTPUTS; i++) {
      if(probabilities[i] > max_logit) {
          max_logit = probabilities[i];
      }
  }
  
  // 2. Calculate exponentials and their sum
  float sum_exp = 0.0;
  for(int i = 0; i < NUMBER_OF_OUTPUTS; i++) {
      probabilities[i] = exp(probabilities[i] - max_logit);
      sum_exp += probabilities[i];
  }
  
  // 3. Normalize to get final 0.00 - 1.00 percentages
  for(int i = 0; i < NUMBER_OF_OUTPUTS; i++) {
      probabilities[i] /= sum_exp;
  }
  
  // Phase 4: Data Extraction
  int predicted_class = ml.probaToClass(probabilities);
  float confidence = probabilities[predicted_class];
  
  Serial.print("Inference Result -> Class: "); 
  Serial.print(predicted_class);
  Serial.print(" | True Confidence Score: "); 
  Serial.println(confidence);
  
  // Phase 5: Hardware Actuation (LED Diagnostic Mode)
  if (predicted_class == 0 && confidence > 0.50) {
    Serial.println("Actuation: FRONT command recognized. Illuminating Front LED.");
    digitalWrite(frontLedPin, HIGH);
    delay(1000); 
    digitalWrite(frontLedPin, LOW);
  } 
  else if (predicted_class == 1 && confidence > 0.50) {
    Serial.println("Actuation: REVERSE command recognized. Illuminating Reverse LED.");
    digitalWrite(reverseLedPin, HIGH);
    delay(1000); 
    digitalWrite(reverseLedPin, LOW);
  } 
  else if (predicted_class == 2 && confidence > 0.50) {
    Serial.println("Actuation: NOISE recognized. Illuminating Noise LED.");
    digitalWrite(noiseLedPin, HIGH);
    delay(1000); 
    digitalWrite(noiseLedPin, LOW);
  } 
  else {
    Serial.println("Actuation: Confidence below 0.50 threshold. Blinking Status LED to indicate failure.");
    for (int blink = 0; blink < 3; blink++) {
      digitalWrite(statusLedPin, HIGH);
      delay(150);
      digitalWrite(statusLedPin, LOW);
      delay(150);
    }
  }
  
  delay(500); 
}