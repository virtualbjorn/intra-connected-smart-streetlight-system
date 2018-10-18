#include <SPI.h>
#include <RH_RF69.h>

#define RF69_FREQ 433.0

#if defined (__AVR_ATmega328P__)  // Feather 328P w/wing
#define RFM69_INT     3  // 
#define RFM69_CS      4  //
#define RFM69_RST     2  // "A"
#endif

RH_RF69 rf69(RFM69_CS, RFM69_INT);

int16_t packetnum = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather RFM69 TX Test!");
  Serial.println();
  if (!rf69.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  rf69.setTxPower(20, true); // range from 14-20 for power, 2nd arg must be true for 69HCW

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
                  };
  rf69.setEncryptionKey(key);
  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
}

void loop() {
  delay(1000);
  // put your main code here, to run repeatedly:
  char x = 'x';
  if (Serial.available() > 0) {
    x = Serial.read();
    Serial.println(x);
  }
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
  if (rf69.waitAvailableTimeout(100)) {
    // Should be a message for us now


  }

  if (x != 'x')
  {
    Serial.println("Button pressed!");

    char radiopacket[20] = "HELLO!";

    Serial.print("Sending "); Serial.println(radiopacket);
    rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
    rf69.waitPacketSent();
  }
}
