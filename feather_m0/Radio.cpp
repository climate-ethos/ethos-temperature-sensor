#include "Radio.h"

// Allows dtostrf
#include <avr/dtostrf.h>

// Radio imports
#include <SPI.h>

// Radio encryption
#include <Crypto.h>
#include <AES.h> // from 'Crypto' library

AES128 aes;
// TODO: Make key secure and move to config file
byte key[] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };

// Configure pins for feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

Radio::Radio(float frequency): _rf95(RFM95_CS, RFM95_INT)
{
  _frequency = frequency;
}

void Radio::setupRadio()
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  delay(1000); // Wait for console
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!_rf95.init()) {
    // Turn on LED to indicate failure
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    delay(50);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init: modulation GFSK_Rb250Fd250, +13dbM
  while (!_rf95.setFrequency(_frequency)) {
    // Turn on LED to indicate failure
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("setFrequency failed");
    delay(50);
  }

  // Clear error light
  digitalWrite(LED_BUILTIN, LOW);

  Serial.print("Set Freq to: ");
  Serial.println(_frequency);

  // Defaults after init are 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  _rf95.setTxPower(23, false);
}

void Radio::sleepRadio() {
  _rf95.sleep();
}

void Radio::sendPacket(float temperatureC, float humidityRH, char sensor_id[3])
{
  // TODO: Better checks for overflows
  // Convert floats to strings
  char temperatureCString[5];
  char humidityRHString[5];
  dtostrf(temperatureC, 4, 1, temperatureCString);
  dtostrf(humidityRH, 4, 1, humidityRHString);
  // Sanitize to ensure that char array end is marked
  temperatureCString[4] = 0;
  humidityRHString[4] = 0;

  // Radio packet looks like "IxxxTxxxxHxxxx0"
  // e.g. "I001T27.2H30.7" where I is ID, T is temperature and H is humidity
  char radioPacket[16]; // 16 bytes equals AES block size
  radioPacket[0] = 'I';
  radioPacket[1] = sensor_id[0];
  radioPacket[2] = sensor_id[1];
  radioPacket[3] = sensor_id[2];
  radioPacket[4] = 'T';
  radioPacket[5] = temperatureCString[0];
  radioPacket[6] = temperatureCString[1];
  radioPacket[7] = temperatureCString[2];
  radioPacket[8] = temperatureCString[3];
  radioPacket[9] = 'H';
  radioPacket[10] = humidityRHString[0];
  radioPacket[11] = humidityRHString[1];
  radioPacket[12] = humidityRHString[2];
  radioPacket[13] = humidityRHString[3];
  radioPacket[14] = 0;
  radioPacket[15] = 0; // set last char to 0
  Serial.println(radioPacket);

  // Encrypt radio packet before sending
  uint8_t encryptedPacket[16];
  aes.setKey(key, sizeof(key));
  aes.encryptBlock(encryptedPacket, (uint8_t *)radioPacket);
  // Print encrypted packet in hexadecimal format
  for (int i = 0; i < 16; i++) {
      Serial.print(encryptedPacket[i], HEX);
      Serial.print(" ");
  }
  Serial.println();

  Serial.println("Sending...");
  _rf95.send((uint8_t *)encryptedPacket, sizeof(encryptedPacket));

  Serial.println("Waiting for packet to complete...");
  delay(10);
  _rf95.waitPacketSent();
}

bool Radio::waitReply()
{
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println("Waiting for reply...");
  // TODO: Optimize time to wait
  if (_rf95.waitAvailableTimeout(1000))
  {
    // Should be a reply message for us now
    if (_rf95.recv(buf, &len))
  {
      Serial.print("Got reply: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(_rf95.lastRssi(), DEC);
      return true;
    }
    else
    {
      Serial.println("Receive failed");
      return false;
    }
  }
  else
  {
    Serial.println("No reply from gateway, retrying transmit");
    return false;
  }
  return false;
}
