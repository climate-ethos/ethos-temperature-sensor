#!/bin/bash

sketch_path="feather_m0/feather_m0.ino"
board="adafruit:samd:adafruit_feather_m0"

# Automatically get the correct port
port=$(arduino-cli board list | grep "Feather M0" | awk '{print $1}')

echo "Detected Feather M0 port as: $port"

while true; do
    read -p "Enter the sensor ID and hit enter to upload or type 'n' to cancel: " sensor_id
    if [ "$sensor_id" = "n" ]; then
        exit 0
    fi

    # Modify the sketch file
    echo "Setting sensor ID to: $sensor_id..."
    gsed -i "s/int sensor_id = [0-9]\+;/int sensor_id = $sensor_id;/" "$sketch_path"

    # Check if sed modified the file
    if ! grep -q "int sensor_id = $sensor_id;" "$sketch_path"; then
        echo "Error: Failed to update sensor ID in $sketch_path"
        continue
    fi

    # Compile and upload
	echo "Compiling sketch..."
    if arduino-cli compile --fqbn $board "$sketch_path"; then
    	echo "Uploading to $port with sensor ID $sensor_id"
        if arduino-cli upload -p $port --fqbn $board "$sketch_path"; then
            echo "Successfully uploaded to $port with sensor ID $sensor_id"
            echo ""
            echo "CHECK SENSOR ID STICKER IS: $sensor_id"
            echo ""
        else
            echo "Upload failed"
        fi
    else
        echo "Compilation failed"
    fi
done