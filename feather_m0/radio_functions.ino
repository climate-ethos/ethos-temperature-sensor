// Allows dtostrf
#include <avr/dtostrf.h>

// Radio imports
#include <SPI.h>
#include <RH_RF95.h>

// Configure feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

// Radio frequency in Mhz, must match receiver
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Setup the radio module
void setupRadio()
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  delay(1000); // Wait for console
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init: modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }

  Serial.print("Set Freq to: ");
  Serial.println(RF95_FREQ);

  // Defaults after init are 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  // Put radio to sleep
  rf95.sleep();
}

// Function that sends packet to the gateway
void sendPacket(float temperatureC, float humidityRH, char sensor_id[3])
{
  // Convert floats to strings
  char temperatureCString[4];
  dtostrf(temperatureC, 4, 1 ,temperatureCString);
  char humidityRHString[4];
  dtostrf(humidityRH, 4, 1 ,humidityRHString);

  // Radio packet looks like "IxxxTxxxxHxxxx0"
  // e.g. "I001T27.2H30.7" where I is ID, T is temperature and H is humidity
  char radiopacket[15];
  radiopacket[0] = 'I';
  strcat(radiopacket, sensor_id);
  radiopacket[4] = 'T';
  strcat(radiopacket, temperatureCString);
  radiopacket[9] = 'H';
  strcat(radiopacket, humidityRHString);
  radiopacket[14] = 0; // set last char to 0

  Serial.println("Sending...");
	// TODO: Is this needed?
  delay(10);
  rf95.send((uint8_t *)radiopacket, 20);

  Serial.println("Waiting for packet to complete...");
  delay(10);
  rf95.waitPacketSent();
}

// Wait for confirmation of response from gateway. If no reply return false
bool waitReply()
{
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println("Waiting for reply...");
  if (rf95.waitAvailableTimeout(1000))
  {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len))
   {
      Serial.print("Got reply: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
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