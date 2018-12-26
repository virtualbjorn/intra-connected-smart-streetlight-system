#include <SPI.h>
#include <RH_RF69.h>
#include <avr/wdt.h>

#define RF69_FREQ 433.0

#if defined (__AVR_ATmega328P__)  // Feather 328P w/wing
#define RFM69_INT     3  // 
#define RFM69_CS      4  //
#define RFM69_RST     2  // "A"
#endif

RH_RF69 rf69(RFM69_CS, RFM69_INT);

const int sensorPin = 5;
const int ledPin =  10;
const int nmiPin = 7;

unsigned long resetTime = 60000; // trigger WDT after 1 minute

int buttonState = 0;
int isSensorData = 0;
int isLoRaData = 0;
int ledBrightness = 20;

void setup() {
  wdt_disable();
  Serial.begin(9600);
  Serial.println("-----------------------");

  pinMode(ledPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  pinMode(nmiPin, INPUT);
  pinMode(RFM69_RST, OUTPUT);

  digitalWrite(RFM69_RST, LOW);

  Serial.println("Booting System");

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  wdt_enable(WDTO_8S);

  if (!rf69.init()) {
    Serial.println("RFM69 radio init failed");
    nmiTrigger();
  }
  Serial.println("RFM69 radio init OK!");
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
    nmiTrigger();
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(14, true);

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
                  };
  rf69.setEncryptionKey(key);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
}

void loop()
{
  buttonState = digitalRead(nmiPin);
  isSensorData = digitalRead(sensorPin);
  if (buttonState == 1) {
    nmiTrigger();
  }
  if (rf69.waitAvailableTimeout(500)) {
    // Should be a message for us now
    wdt_reset();
    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (! rf69.recv(buf, &len)) {
      Serial.println("Receive failed");
      return;
    }
    rf69.printBuffer("Received: ", buf, len);
    buf[len] = 0;
    Serial.print("Got: "); Serial.println((char*)buf);
    Serial.print("RSSI: "); Serial.println(rf69.lastRssi(), DEC);
    isLoRaData = 1;
    switchMode(isSensorData, isLoRaData);
  } else {
    if (isSensorData == 1) {
      wdt_reset();
      char radiopacket[20] = "1";
      Serial.println("Sending  data to next node");
      rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
      rf69.waitPacketSent();
    }
    if (millis() <= resetTime) {
      wdt_reset();
    } else if (isSensorData == 0 and isLoRaData == 0) {
      Serial.println("System will reboot after not detecting anything in the next 8 seconds...");
    }
    switchMode(isSensorData, isLoRaData);
    isLoRaData = 0;
  }
  delay(500);
}

void switchMode(int sensorData, int loRaData) {
  if (sensorData == 1 or loRaData == 1) {
    Serial.println("Switching to Bright Mode");
    for (int i = ledBrightness; i <= 255; i++) {
      analogWrite(ledPin, i);
      delay(5);
      if (i == 255) {
        ledBrightness = i;
      }
    }
  } else {
    Serial.println("Switching to Dim Mode");
    for (int i = ledBrightness; i >= 20; i--) {
      analogWrite(ledPin, i);
      delay(5);
      if (i == 20) {
        ledBrightness = i;
      }
    }
  }
}

void nmiTrigger() {
  Serial.println("NMI Triggered. Rebooting System");
  wdt_disable();
  wdt_enable(WDTO_60MS);
}
