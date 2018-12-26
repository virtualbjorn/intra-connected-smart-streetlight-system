#include <MsTimer2.h>           //Timer interrupt function library
#include "PinChangeInterrupt.h"
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

const int ledPin =  10;
int pbIn = 7;                    // Define interrupt 0 that is digital pin 2
int ledOut = 13;                 // Define the indicator LED pin digital pin 13
int number = 0;                  //Interrupt times
volatile int state = LOW;         // Defines the indicator LED state, the default is not bright

unsigned long resetTime = 60000; // trigger WDT after 1 minute

int isSensorData = 0;
int isLoRaData = 0;
int ledBrightness = 20;

void setup()
{
  Serial.begin(9600);
  Serial.println("-----------------------");

  pinMode(ledPin, OUTPUT);
  pinMode(RFM69_RST, OUTPUT);

  pinMode(ledOut, OUTPUT);
  attachPinChangeInterrupt(digitalPinToPCINT(pbIn), stateChange, FALLING);
  //    attachInterrupt(pbIn, stateChange, FALLING); // Set the interrupt function, interrupt pin is digital pin D2, interrupt service function is stateChange (), when the D2 power change from high to low , the trigger interrupt.
  MsTimer2::set(500, Handle); // Set the timer interrupt function, running once Handle() function per 1000ms
  MsTimer2::start();//Start timer interrupt function

  digitalWrite(RFM69_RST, LOW);

  Serial.println("Booting System");
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
  if (number > 1) {
    //    Serial.println(number); // Printing the number of times of interruption, which is convenient for debugging.
  }

  delay(1);
  if (millis() <= resetTime) {
    wdt_reset();
  } else if (isSensorData == 0 and isLoRaData == 0) {
    Serial.println("System will reboot after not detecting anything in the next 8 seconds...");
  }
  if (state == HIGH) //When a moving object is detected, the ledout is automatically closed after the light 2S, the next trigger can be carried out, and No need to reset. Convenient debugging.
  {
    delay(1000);
    state = LOW;
    digitalWrite(ledOut, state);    //turn off led
  }

}


void stateChange()  //Interrupt service function
{
  number++;  //Interrupted once, the number +1
}

void Handle()   //Timer service function
{
  if (number > 1) //If in the set of the interrupt time the number more than 1 times, then means have detect moving objects,This value can be adjusted according to the actual situation, which is equivalent to adjust the threshold of detection speed of moving objects.
  {
    state = HIGH;

    Serial.println("Object Detected");

    char radiopacket[20] = "1";
    Serial.println("?Sending  data to next node");
    rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
    rf69.waitPacketSent();

    digitalWrite(ledOut, state);    //light led
    number = 0; //Clear the number, so that it does not affect the next trigger

    wdt_reset();
  }
  else
    number = 0; //If in the setting of the interrupt time, the number of the interrupt is not reached the threshold value, it is not detected the moving objects, Cleare the number.
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
