@echo off

:: Step 1: Verify Arduino CLI installation
echo Verifying Arduino CLI installation...
where arduino-cli.exe >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo arduino-cli.exe not found. Please install it before running this script.
    exit /B 1
) else (
    arduino-cli.exe version
)

:: Step 2: Initialize Arduino CLI config
echo Initializing Arduino CLI configuration...
arduino-cli.exe config init

:: Step 3: Add Adafruit board manager URL
echo Adding Adafruit board manager URL...
arduino-cli.exe config add board_manager.additional_urls https://adafruit.github.io/arduino-board-index/package_adafruit_index.json

:: Step 4: Install Arduino SAMD Boards
echo Installing Arduino SAMD Boards...
arduino-cli.exe core install arduino:samd

:: Step 5: Install Adafruit SAMD Boards
echo Installing Adafruit SAMD Boards...
arduino-cli.exe core install adafruit:samd

echo Arduino CLI setup complete!
pause
