#!/bin/bash

# Step 1: Verify Arduino CLI installation
echo "Verifying Arduino CLI installation..."
if ! command -v arduino-cli &> /dev/null
then
    echo "arduino-cli not found. Please install it before running this script."
    exit 1
else
    arduino-cli version
fi

# Step 2: Initialize Arduino CLI config
echo "Initializing Arduino CLI configuration..."
arduino-cli config init

# Step 3: Add Adafruit board manager URL
echo "Adding Adafruit board manager URL..."
arduino-cli config add board_manager.additional_urls https://adafruit.github.io/arduino-board-index/package_adafruit_index.json

# Step 4: Install Arduino SAMD Boards
echo "Installing Arduino SAMD Boards..."
arduino-cli core install arduino:samd

# Step 5: Install Adafruit SAMD Boards
echo "Installing Adafruit SAMD Boards..."
arduino-cli core install adafruit:samd

echo "Arduino CLI setup complete!"
