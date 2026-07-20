#include <EloquentTinyML.h>
#include "model_data.h" 

#define NUMBER_OF_INPUTS 200
#define NUMBER_OF_OUTPUTS 3
#define TENSOR_ARENA_SIZE 16384 // Required memory for your 100-neuron layer

Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;

// ESP8266 GPIO Mappings
const int micPin = A0;  // In the ESP8266 core, the TOUT pin is addressed as A0
const int buttonPin = 4; // GPIO4
const int leftMotorForward = 2; // GPIO2

float features[NUMBER_OF_INPUTS];
float input_tensor[NUMBER_OF_INPUTS]; 

int ambientNoiseAverage = 0;
int threshold = 0;
const int thresholdMargin = 15;

void setup() {
  Serial.begin(115200);
  
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(leftMotorForward, OUTPUT);
  
  ml.begin(acoustic_nn_model);
  
  delay(1000);
  calibrateEnvironment();
}

void calibrateEnvironment() {
  long sumOfReadings = 0;
  for (int i = 0; i < 500; i++) {
    sumOfReadings += analogRead(micPin);
    delay(2); 
  }
  ambientNoiseAverage = sumOfReadings / 500;
  threshold = ambientNoiseAverage + thresholdMargin;
}

void loop() {
  while (digitalRead(buttonPin) == HIGH) {
    // Wait for physical button press
  }
  
  // 1. CAPTURE & SCALE PHASE
  for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
    int raw_val = analogRead(micPin) - threshold;
    
    // Apply the exact same Global Math Scaling used in Python
    input_tensor[i] = (float)raw_val / 512.0; 
    delay(5); 
  }
  
  // 2. INFERENCE PHASE
  float probabilities[NUMBER_OF_OUTPUTS]; 
  ml.predict(input_tensor, probabilities);
  int predicted_class = ml.probaToClass(probabilities);
  
  Serial.print("Predicted Class: ");
  Serial.print(predicted_class);
  Serial.print(" | Confidence: ");
  Serial.println(probabilities[predicted_class]);
  
  // 3. HARDWARE EXECUTION PHASE
  if (predicted_class == 0 && probabilities[0] > 0.70) {
    digitalWrite(leftMotorForward, HIGH);
    delay(1000); 
    digitalWrite(leftMotorForward, LOW);
  } 
  
  delay(500);
}