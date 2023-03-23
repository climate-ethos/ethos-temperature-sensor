// Import for sleep
#include <RTCZero.h>

#include "Radio.h"

// The ID of the sensor, change depending on what number to assign
char sensor_id[] = "001";

// TODO: Change default frequency of radio
float radio_frequency = 915.0;
Radio radio(radio_frequency);

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

  // Begin console
  Serial.begin(115200);

  // Setup radio
  radio.setupRadio();
  radio.sleepRadio();

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
    // TODO: Read sensor data measurement
    float temperatureC = 27.3;
    float humidityRH = 30.2;
    // Send packet to gateway
    radio.sendPacket(temperatureC, humidityRH, sensor_id);
    // After 3 unsuccessful transmits (no reply from gateway), give up
    int numberOfRetries = 0;
    while (!radio.waitReply() && numberOfRetries < 2)
    {
      radio.sendPacket(temperatureC, humidityRH, sensor_id);
      numberOfRetries++;
    }
  }
  // Reset alarm and return to sleep
  radio.sleepRadio();
  resetAlarm();
  zerortc.standbyMode();
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
