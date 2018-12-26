const int sensorPin = 2;
const int ledPin =  9;
int ledBrightness = 255;
void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(sensorPin, INPUT);
}
void loop()
{
  if (digitalRead(sensorPin) == HIGH)
  {
    Serial.println("Detected!");
    for (int i = ledBrightness; i <= 255; i++) {
      analogWrite(ledPin, i);
      delay(5);
      if(i == 255) {
        ledBrightness = i;
      }
    }
  }
  else {
    Serial.println("Not Detected!");
    for (int i = ledBrightness; i >= 100; i--) {
      analogWrite(ledPin, i);
      delay(5);
      if(i == 100) {
        ledBrightness = i;
      }
    }
  }
  delay(1000);
}
