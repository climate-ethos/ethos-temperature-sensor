#include "Radio.h"
#include "config.h"

// Allows dtostrf
#include <avr/dtostrf.h>

// Radio imports
#include <SPI.h>

// Radio encryption
#include <Crypto.h>
#include <AES.h> // from 'Crypto' library

AES128 aes;

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

  // Add retry loop to prevent hang if radio is not found
  int retries = 0;
  while (!_rf95.init() && retries < 10) {
    // Turn on LED to indicate failure
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    delay(500);
    retries++;
  }

  // If it still failed, halt. The Watchdog Timer will reset the device.
  if (retries >= 10) {
    Serial.println("FATAL: Radio init failed. Halting.");
    while (1);
  }

  Serial.println("LoRa radio init OK!");

  // Defaults after init: modulation GFSK_Rb250Fd250, +13dbM
  // Add retry loop
  retries = 0;
  while (!_rf95.setFrequency(_frequency) && retries < 10) {
    // Turn on LED to indicate failure
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("setFrequency failed");
    delay(500);
    retries++;
  }

  // If it still failed, halt. The Watchdog Timer will reset the device.
  if (retries >= 10) {
    Serial.println("FATAL: Radio setFrequency failed. Halting.");
    while (1);
  }

  // Clear error light
  digitalWrite(LED_BUILTIN, LOW);

  Serial.print("Set Freq to: ");
  Serial.println(_frequency);

  // Defaults after init are 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:

  // Reduced power from 23 to 20. 23dBm is very high and can
  // cause voltage brownouts on battery, crashing the MCU.
  // 20dBm is still very high power.
  _rf95.setTxPower(20, false);
}

void Radio::sleepRadio() {
  _rf95.sleep();
}

void Radio::sendPacket(int sensorId, float temperatureC, float humidityRH, float batteryVoltage)
{
  // Radio packet should look like "IIIITTTTHHHHVVVV"
  // Where I is ID, T is temperature, H is humidity and V is voltage
  // This assumes a 4 byte integer
  uint8_t radioPacket[16]; // 16 bytes equals AES block size

  memcpy(&radioPacket[0], &sensorId, 4);
  memcpy(&radioPacket[4], &temperatureC, 4);
  memcpy(&radioPacket[8], &humidityRH, 4);
  memcpy(&radioPacket[12], &batteryVoltage, 4);

  // Encrypt radio packet before sending
  uint8_t encryptedPacket[16];
  aes.setKey(reinterpret_cast<const uint8_t*>(KEY_STRING), sizeof(KEY_STRING) - 1);  // -1 to exclude the null terminator
  aes.encryptBlock(encryptedPacket, radioPacket);

  Serial.println("Sending...");
  _rf95.send((uint8_t *)encryptedPacket, sizeof(encryptedPacket));

  Serial.println("Waiting for packet to complete...");
  delay(10);

  // Added a 10-second (10000ms) timeout to waitPacketSent().
  // The original call had no timeout and was the most likely
  // cause of your device hanging.
  if (!_rf95.waitPacketSent(10000)) {
    Serial.println("WARNING: waitPacketSent() timed out. Radio may be hung.");
  } else {
    Serial.println("Packet sent.");
  }
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