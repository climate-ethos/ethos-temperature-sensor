// Import for sleep
#include <RTCZero.h>

// Library for SHT4x
#include "Adafruit_SHT4x.h"

// Custom library for radio
#include "Radio.h"

// Library for Watchdog Timer
#include <Adafruit_SleepyDog.h>

// Define SHT power pin
#define SHT_PWD_PIN A0

// Define battery pin
#define VBATPIN A7

#if defined (MOTEINO_M0)
  #if defined(SERIAL_PORT_USBVIRTUAL)
    #define Serial SERIAL_PORT_USBVIRTUAL // Required for Serial on Zero based boards
  #endif
#endif

// The ID of the sensor, change depending on what number to assign
// This should match the sticker on the sensor
int sensor_id = 1;
// Change default frequency of radio
float radio_frequency = 915.1;
Radio radio(radio_frequency);

// RTC Clock for sleep
RTCZero zerortc;
// How often to take temperature measurements
const byte alarmSeconds = 0;
const byte alarmMinutes = 10;
const byte alarmHours = 0;

volatile bool alarmFlag = true; // Start awake

// Setup temp sensor
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
void setup()
{
  // Enable the Watchdog Timer.
  // The second parameter is 'enable_in_sleep'. We set it to 'false' (or omit it)
  // so the WDT will PAUSE when the device is in standbyMode.
  // This is the fix for the 1-second reset loop.
  int wdt_timeout = Watchdog.enable(8000, false);

  // Begin console
  Serial.begin(115200);

  // Wait 0.5 seconds (500ms) for Serial port to connect.
  long setup_start = millis();
  while (!Serial && (millis() - setup_start < 500));

  Serial.print("Watchdog enabled with ");
  Serial.print(wdt_timeout);
  Serial.println("ms timeout. WDT will PAUSE during sleep.");

  // Turn off LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  // Setup A0 to power output
  digitalWrite(SHT_PWD_PIN, LOW);
  pinMode(SHT_PWD_PIN, OUTPUT);

  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
  Serial.println("Configured sensor");

  // Setup radio
  radio.setupRadio();
  radio.sleepRadio();

  // Setup clocks
  zerortc.begin();

  // Set the clock to 0 ONCE, here in setup.
  // Do not call this in the loop.
  zerortc.setTime(0, 0, 0);
  zerortc.setDate(1, 1, 1);

  // Set alarm
  resetAlarm();
  // Setup handler for alarm
  zerortc.attachInterrupt(alarmMatch);
  Serial.println(sensor_id);
}

void loop()
{
  // Woken up from sleep
  if (alarmFlag == true) {
    alarmFlag = false;  // Clear flag

    // Read battery voltage first, so we can send it even if sensor fails
    u_int32_t raw_voltage_value = analogRead(VBATPIN);
    float battery_voltage = raw_voltage_value * 2;
    // Divided by 2, so multiply back
    battery_voltage *= 3.3;
    // Multiply by 3.3V (reference voltage)
    battery_voltage /= 1024;
    // Convert to voltage
    // Ensure battery voltage is within a realistic range
    if (battery_voltage <= 0.0 || battery_voltage > 5.0) {
      battery_voltage = 1; // Default safe value
    }

    // Turn on sensor
    digitalWrite(SHT_PWD_PIN, HIGH);
    delay(10);
    // Setup temp sensor
    int num_retries = 0;
    while (!sht4.begin() && num_retries < 10) {
      num_retries++; // Prevent infinite loop
      delay(15);
    }

    // Read sensor data measurement
    sensors_event_t humidity, temperature;

    if (num_retries >= 10) {
      Serial.println("Unable to setup sensor");
      // Set to NAN to indicate sensor failure
      temperature.temperature = NAN;
      humidity.relative_humidity = NAN;
    } else {
      num_retries = 0;
      while(!sht4.getEvent(&humidity, &temperature) && num_retries < 10) {
        num_retries++; // Prevent infinite loop
        delay(15);
      }

      if (num_retries >= 10 || isnan(humidity.relative_humidity) || isnan(temperature.temperature)) {
        Serial.println("Unable to get temp/humidity measurement");
        // Set to NAN to indicate sensor failure
        temperature.temperature = NAN;
        humidity.relative_humidity = NAN;
      }
    }

    // Turn off sensor
    digitalWrite(SHT_PWD_PIN, LOW);

    // Send packet to gateway, even if sensor reading failed
    // (sends NAN values)
    radio.sendPacket(sensor_id, temperature.temperature, humidity.relative_humidity, battery_voltage);
    delay(10); // This is needed to prevent hanging
  }

  // Reset alarm and return to sleep
  sleepEverything();
}

void sleepEverything(void) {
  // Turn off sensor
  digitalWrite(SHT_PWD_PIN, LOW);
  // Turn off radio
  radio.sleepRadio();
  // Reset alarm and return to sleep
  resetAlarm();

  // "Pet" the watchdog right before sleeping
  // If the RTC fails to wake us up, the WDT will reset the device
  Watchdog.reset();

  zerortc.standbyMode();
}

void alarmMatch(void) {
  alarmFlag = true; // Set flag
}

// This function now sets the alarm 10 minutes *relative* to the
// current RTC time, which is the robust way to do it.
void resetAlarm(void) {
  // Get the current time from the RTC
  byte seconds = zerortc.getSeconds();
  byte minutes = zerortc.getMinutes();
  byte hours = zerortc.getHours();

  // Add the alarm offset (10 minutes, 0 seconds)
  seconds += alarmSeconds;
  minutes += alarmMinutes;
  hours += alarmHours;

  // Handle the "rollover"
  if (seconds >= 60) {
    seconds -= 60;
    minutes++;
  }
  if (minutes >= 60) {
    minutes -= 60;
    hours++;
  }
  if (hours >= 24) {
    hours -= 24;
    // Note: This doesn't handle date rollover, but for a simple
    // 10-minute timer, this is perfectly fine.
  }

  // Set the alarm for the new, calculated time
  zerortc.setAlarmTime(hours, minutes, seconds);
  // Enable the alarm to match Hours, Minutes, and Seconds
  zerortc.enableAlarm(zerortc.MATCH_HHMMSS);
}