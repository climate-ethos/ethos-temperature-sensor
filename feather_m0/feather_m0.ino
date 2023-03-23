// Allows dtostrf
#include <avr/dtostrf.h>

// Radio imports
#include <SPI.h>
#include <RH_RF95.h>

// Import for sleep
#include <RTCZero.h>

// The ID of the sensor, change depending on what number to assign
const char sensor_id[] = "001";

// Configure feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

// Radio frequency in Mhz, must match receiver
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// RTC Clock for sleep
RTCZero zerortc;

// Set how often alarm goes off here
// TODO: Change to every 10 minutes
const byte alarmSeconds = 10;
const byte alarmMinutes = 0;
const byte alarmHours = 0;

volatile bool alarmFlag = false; // Start awake

#if defined (MOTEINO_M0)
  #if defined(SERIAL_PORT_USBVIRTUAL)
    #define Serial SERIAL_PORT_USBVIRTUAL // Required for Serial on Zero based boards
  #endif
#endif

void setup()
{
  // Turn off LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  // Setup radio
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
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

  // Setup clocks
  zerortc.begin();

  // Set alarm
  resetAlarm();
  // Setup handler for alarm
  zerortc.attachInterrupt(alarmMatch);
}

void loop()
{
  // Woken up from sleep
  if (alarmFlag == true) {
    alarmFlag = false;  // Clear flag
    // Send packet to gateway
    sendPacket();
    // After 3 unsuccessful transmits (no reply from gateway), give up
    int numberOfRetries = 0;
    while (!waitReply() && numberOfRetries < 2)
    {
      sendPacket();
      numberOfRetries++;
    }
  }
  // Reset alarm and return to sleep
  resetAlarm();
  zerortc.standbyMode();
}

// Function that sends packet to the gateway
void sendPacket()
{
  Serial.println("Transmitting..."); // Send a message to rf95_server
  // TODO: Measure temp and humidity
  float temperatureC = 27.2;
  char temperatureCString[4];
  dtostrf(temperatureC, 4, 1 ,temperatureCString);
  float humidityRH = 30.7;
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

void alarmMatch(void)
{
  alarmFlag = true; // Set flag
}

void resetAlarm(void) {
  byte seconds = 0;
  byte minutes = 0;
  byte hours = 0;
  byte day = 1;
  byte month = 1;
  byte year = 1;

  zerortc.setTime(hours, minutes, seconds);
  zerortc.setDate(day, month, year);

  zerortc.setAlarmTime(alarmHours, alarmMinutes, alarmSeconds);
  zerortc.enableAlarm(zerortc.MATCH_HHMMSS);
}
