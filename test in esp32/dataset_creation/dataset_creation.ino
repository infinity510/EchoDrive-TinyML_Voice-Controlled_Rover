#define NUMBER_OF_INPUTS 200

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
  for (int i = 0; i < 500; i++)
  {
    sumOfReadings += analogRead(micPin);
    delay(2);
  }
  threshold = (sumOfReadings / 500) + thresholdMargin;
}

void loop()
{
  while (digitalRead(buttonPin) == HIGH)
  {
    delay(10);
  }

  digitalWrite(statusLedPin, HIGH);

  int raw_capture[NUMBER_OF_INPUTS];
  for (int i = 0; i < NUMBER_OF_INPUTS; i++)
  {
    raw_capture[i] = analogRead(micPin) - threshold;
    delay(5);
  }

  digitalWrite(statusLedPin, LOW);

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