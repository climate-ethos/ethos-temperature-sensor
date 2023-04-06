#include "Radio.h"

// Allows dtostrf
#include <avr/dtostrf.h>

// Radio imports
#include <SPI.h>

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
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init: modulation GFSK_Rb250Fd250, +13dbM
  if (!_rf95.setFrequency(_frequency)) {
    Serial.println("setFrequency failed");
    while (1);
  }

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
  char radiopacket[15];
  radiopacket[0] = 'I';
  radiopacket[1] = sensor_id[0];
  radiopacket[2] = sensor_id[1];
  radiopacket[3] = sensor_id[2];
  radiopacket[4] = 'T';
  radiopacket[5] = temperatureCString[0];
  radiopacket[6] = temperatureCString[1];
  radiopacket[7] = temperatureCString[2];
  radiopacket[8] = temperatureCString[3];
  radiopacket[9] = 'H';
  radiopacket[10] = humidityRHString[0];
  radiopacket[11] = humidityRHString[1];
  radiopacket[12] = humidityRHString[2];
  radiopacket[13] = humidityRHString[3];
  radiopacket[14] = 0; // set last char to 0
  Serial.println(radiopacket);

  // TODO: Encrypt radio packet before sending

  Serial.println("Sending...");
  _rf95.send((uint8_t *)radiopacket, 20);

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
