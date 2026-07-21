// ==========================================
// ESP32: HIGH-RESOLUTION DATA LOGGER
// ==========================================
#define NUMBER_OF_INPUTS 800

const int micPin = 34;
const int buttonPin = 32;
const int statusLedPin = 13;

int threshold = 0;
const int thresholdMargin = 15;

void setup()
{
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(statusLedPin, OUTPUT);
  digitalWrite(statusLedPin, LOW);

  calibrateEnvironment();
}

void calibrateEnvironment()
{
  long sumOfReadings = 0;
  for (int i = 0; i < 800; i++)
  {
    sumOfReadings += analogRead(micPin);
    delay(2);
  }
  threshold = (sumOfReadings / 800) + thresholdMargin;
  Serial.println("SYSTEM_READY");
}

void loop()
{
  // 1. Wait for physical trigger
  while (digitalRead(buttonPin) == HIGH)
  {
    delay(10);
  }

  // 2. Pre-recording sequence
  digitalWrite(statusLedPin, HIGH);
  Serial.println("Start speaking");
  delay(50); // 50ms buffer as requested

  // 3. Audio Capture (800 samples over 1.0 seconds)
  int raw_capture[NUMBER_OF_INPUTS];
  for (int i = 0; i < NUMBER_OF_INPUTS; i++)
  {
    raw_capture[i] = analogRead(micPin) - threshold;
    delayMicroseconds(1250);
  }

  digitalWrite(statusLedPin, LOW);

  // 4. Data Transmission to Python
  for (int i = 0; i < NUMBER_OF_INPUTS; i++)
  {
    Serial.print(raw_capture[i]);
    if (i < NUMBER_OF_INPUTS - 1)
    {
      Serial.print(",");
    }
  }
  Serial.println();

  delay(1000);
}