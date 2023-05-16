// Import for sleep
#include <RTCZero.h>

// Library for SHT4x
#include "Adafruit_SHT4x.h"

// Custom library for radio
#include "Radio.h"

// Define SHT power pin
#define SHT_PWD_PIN A0

#if defined (MOTEINO_M0)
  #if defined(SERIAL_PORT_USBVIRTUAL)
    #define Serial SERIAL_PORT_USBVIRTUAL // Required for Serial on Zero based boards
  #endif
#endif

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

volatile bool alarmFlag = true; // Start awake

// Setup temp sensor
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

void setup()
{
  // Begin console
  Serial.begin(115200);
  while (!Serial); // Wait for serial connection

  // Turn off LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Setup A0 to power output
  digitalWrite(SHT_PWD_PIN, LOW);
  pinMode(SHT_PWD_PIN, OUTPUT);
  digitalWrite(SHT_PWD_PIN, HIGH);
  delay(1);

  // Setup temp sensor
  while (!sht4.begin()) {
    Serial.println("Couldn't find SHT4x");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(10);
  }
  digitalWrite(LED_BUILTIN, LOW);

  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
  Serial.println("Initialized sensor");

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

    // Re-enumerate USB
    USBDevice.detach();
    delay(5000); // You may need to adjust this delay
    USBDevice.attach();
    delay(5000);
    
    // Turn on sensor
    digitalWrite(SHT_PWD_PIN, HIGH);
    delay(1);
    // Read sensor data measurement
    sensors_event_t humidity, temperature;
    int num_retries = 0;
    while(!sht4.getEvent(&humidity, &temperature) && num_retries < 10) {
      num_retries++; // prevent infinite loop
      Serial.println("Retry measure");
      delay(10);
    }
    // Turn off sensor
    digitalWrite(SHT_PWD_PIN, LOW);
    // Send packet to gateway if sensor reading exists
    if (num_retries < 10) {
      radio.sendPacket(temperature.temperature, humidity.relative_humidity, sensor_id);
      delay(10); // This is needed to prevent hanging
      Serial.println("Packet sent!");
      // TODO: If no reply, retry transmit once
      // if (!radio.waitReply())
      // {
      //   radio.sendPacket(temperatureC, humidityRH, sensor_id);
      // }
    } else {
      Serial.println("Unable to get measurement");
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
