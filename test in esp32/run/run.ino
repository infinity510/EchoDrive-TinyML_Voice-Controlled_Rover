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

// The specific class definition path required for EloquentTinyML Version 2.4.4 on ESP32
Eloquent::TinyML::TensorFlow::TensorFlow<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;

// ==========================================
// 3. HARDWARE PIN DEFINITIONS (ESP32)
// ==========================================
const int micPin = 34; 
const int buttonPin = 32; 
const int ledPin = 13; 
const int leftMotorForward = 25; 
const int rightMotorForward = 26;

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
  
  // Downscale ESP32 12-bit ADC to match the Arduino UNO 10-bit training data
  analogReadResolution(10); 
  
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(leftMotorForward, OUTPUT);
  pinMode(rightMotorForward, OUTPUT);
  
  // Bind the compiled C++ array to the Eloquent object
  ml.begin(acoustic_nn_model);
  
  calibrateEnvironment();
}

// ==========================================
// 7. MAIN EXECUTION LOOP
// ==========================================
void loop() {
  // Await user trigger 
  while (digitalRead(buttonPin) == HIGH) {
    delay(10);
  }
  
  digitalWrite(ledPin, HIGH);
  
  // Phase 1: Data Acquisition
  for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
    input_tensor[i] = (float)(analogRead(micPin) - threshold);
    delay(5); 
  }
  
  digitalWrite(ledPin, LOW);
  
  // Phase 2: Neural Network Inference
  float probabilities[NUMBER_OF_OUTPUTS];
  ml.predict(input_tensor, probabilities);
  
  // Phase 3: Data Extraction
  int predicted_class = ml.probaToClass(probabilities);
  float confidence = probabilities[predicted_class];
  
  // Output diagnostic metrics
  Serial.print("Inference Result -> Class: "); 
  Serial.print(predicted_class);
  Serial.print(" | Confidence Score: "); 
  Serial.println(confidence);
  
  // Phase 4: Hardware Actuation
  if (predicted_class == 0 && confidence > 0.60) {
    Serial.println("Actuation: Executing FRONT command logic sequence.");
    digitalWrite(leftMotorForward, HIGH);
    digitalWrite(rightMotorForward, HIGH);
    
    delay(1000); 
    
    digitalWrite(leftMotorForward, LOW);
    digitalWrite(rightMotorForward, LOW);
  }
  
  delay(500); 
}