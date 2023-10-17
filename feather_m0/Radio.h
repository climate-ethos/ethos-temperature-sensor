/*
  Radio.h - Library for handling radio communication
*/

// This is needed in Arduino modules
#include "Arduino.h"

// RH_RF95 type
#include <RH_RF95.h>

class Radio {
  public:
    /**
     * @brief Construct a new Radio object
     *
     * @param frequency The default frequency of the radio communications (Mhz)
     */
    Radio(float frequency);

    /**
     * @brief Setup the radio module
     *
     */
    void setupRadio();

    /**
     * @brief Put the radio to sleep
     *
     */
    void sleepRadio();

    /**
     * @brief Function that sends packet to the gateway
     *
     * @param temperatureC Temperature of reading in C
     * @param humidityRH Relative humidity of reading %
     * @param sensor_id The ID of the sensor that is transmitting info
     */
    void sendPacket(int sensor_id, float temperatureC, float humidityRH, float batteryVoltage);

    /**
     * @brief Wait for confirmation of response from gateway
     *
     * @return true when reply is received from gateway
     * @return false when no reply is received
     */
    bool waitReply();

  private:
    // Holds the rf95 radio class
    RH_RF95 _rf95;
    // Holds the frequency of the radio module
    float _frequency;
};