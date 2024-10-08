# Ethos Temperature Sensor

This is the code which runs on the Feather M0 board to send temperature sensor data to the base station.

## Installation/Setup

1. Install the required libraries from the Arduino IDE
2. Copy `conf.h.example` -> `conf.h` and add your 16 bit radio encryption key
3. Upload code to Feather M0 LoRa board with temperature sensor

## Automatic upload script

1. Install the Arduino CLI (instructions [here](https://arduino.github.io/arduino-cli/0.35/installation/))
2. Run the setup script with `sh arduino_cli_setup.sh` or `arduino_cli_setup.bat` (Windows)
3. Ensure all libraries are in the `Arduino/libraries` path
4. (On MacOS) run `brew install gnu-sed`
5. Run the upload script with `sh mac_sensor_upload.sh` or `windows_sensor_upload.bat`
6. Plug in a device and double tap reset button to allow upload

## Arduino IDE Setup

1. Add the additional board manager url `https://adafruit.github.io/arduino-board-index/package_adafruit_index.json`
2. Install the `Arduino SAMD Boards` from the boards manager.
3. Install the `Adafruit SAMD Boards` from the boards manager.
4. Select the matching board: `Adafruit Feather M0 (SAMD21)`.

## Arduino CLI Setup (Manual)

1. Verify installation with `arduino-cli version`
2. Initialize the Arduino CLI config with `arduino-cli config init`
3. Add the Adafruit board manager url with `arduino-cli config add board_manager.additional_urls https://adafruit.github.io/arduino-board-index/package_adafruit_index.json`
4. Install the Arduino SAMD Boards with `arduino-cli core install arduino:samd`
5. Install the Adafruit SAMD Boards with `arduino-cli core install arduino:samd`
