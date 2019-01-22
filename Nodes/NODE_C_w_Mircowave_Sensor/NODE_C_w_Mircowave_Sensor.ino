#include <MsTimer2.h>
#include "PinChangeInterrupt.h"
#include <SPI.h>
#include <RH_RF69.h>
#include <avr/wdt.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

#define RF69_FREQ 433.0

#if defined(__AVR_ATmega328P__)
#define RFM69_INT 3
#define RFM69_CS 4
#define RFM69_RST 2
#define triggerTime 10000
#define resetTime 360000
#define pbIn 5
#define debugPin 8
#define ledPin 9
#define chipSelect 10
#endif

RTC_DS1307 rtc;

RH_RF69 rf69(RFM69_CS, RFM69_INT);

int num = 0;
volatile int microwaveSensorState = LOW;
volatile int isLoRaData = LOW;

volatile int wdtTrigger = HIGH;

int ledBrightness = 25;
unsigned long lastTriggerTime = 0;

void setup()
{
  // Serial.begin(115200);
  pinMode(RFM69_RST, OUTPUT);
  pinMode(debugPin, OUTPUT);
  digitalWrite(RFM69_RST, LOW);
  attachPCINT(digitalPinToPCINT(pbIn), stateChange, FALLING);
  wdt_enable(WDTO_8S);
  if (!rf69.init())
  {
    digitalWrite(debugPin, HIGH);
    delay(300);
    nmiTrigger();
  }
  if (!rf69.setFrequency(RF69_FREQ))
  {
    nmiTrigger();
  }
  if (!rtc.begin())
  {
    nmiTrigger();
  }
  if (!SD.begin(chipSelect))
  {
    digitalWrite(debugPin, HIGH);
    delay(100);
    nmiTrigger();
  }
  rf69.setTxPower(14, true);
  uint8_t key[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                   0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);
  switchMode(microwaveSensorState, isLoRaData);
  MsTimer2::set(100, Handle);
  MsTimer2::start();
}

void loop()
{
  if (rf69.waitAvailableTimeout(500))
  {
    wdt_reset();
    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (!rf69.recv(buf, &len))
    {
      return;
    }
    buf[len] = 0;
    lastTriggerTime = millis();
    isLoRaData = HIGH;
    digitalWrite(debugPin, HIGH);
    if (strncmp((char *)buf, "NODE B", len) == 0)
    {
      switchMode(microwaveSensorState, isLoRaData);
    }
    isLoRaData = LOW;
  }
  if (microwaveSensorState == HIGH)
  {
    if (ledBrightness == 25)
    {
      char radiopacket[7] = "NODE C";
      rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
      rf69.waitPacketSent();
    }
    switchMode(microwaveSensorState, isLoRaData);
    attachPCINT(digitalPinToPCINT(pbIn), stateChange, FALLING);
    microwaveSensorState = LOW;
    lastTriggerTime = millis();
  }
  else if (ledBrightness != 25 and (millis() - lastTriggerTime >= triggerTime) == 1)
  {
    switchMode(microwaveSensorState, isLoRaData);
  }
  if (millis() <= resetTime)
  {
    wdt_reset();
  }
}

void stateChange()
{
  num++;
}

void Handle()
{
  if (num > 1)
  {
    microwaveSensorState = HIGH;
    detachPCINT(digitalPinToPCINT(pbIn));
    num = 0;
    wdt_reset();
  }
  else
  {
    num = 0;
  }
}

void switchMode(volatile int sensorData, volatile int loRaData)
{
  DateTime now = rtc.now();
  File logFile = SD.open("datalog.txt", FILE_WRITE);
  char timeStamp[20];
  sprintf(timeStamp, "%04u/%02u/%02u-%02u:%02u:%02u", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  logFile.print(timeStamp);
  logFile.print(", ");
  logFile.print(sensorData);
  logFile.print(", ");
  logFile.print(loRaData);
  logFile.print(", ");
  if (sensorData == HIGH or loRaData == HIGH)
  {
    logFile.print("Bright Mode");
    logFile.println("");
    logFile.close();
    digitalWrite(debugPin, HIGH);
    ledBrightness = 255;
    analogWrite(ledPin, ledBrightness);
  }
  else
  {
    logFile.print("Dim Mode");
    logFile.println("");
    logFile.close();
    digitalWrite(debugPin, LOW);
    ledBrightness = 25;
    analogWrite(ledPin, ledBrightness);
  }
}

void nmiTrigger()
{
  wdt_disable();
  wdt_enable(WDTO_60MS);
}
