@echo off
setlocal

set "sketch_path=feather_m0\feather_m0.ino"
set "board=adafruit:samd:adafruit_feather_m0"

rem Automatically get the correct port
for /f "tokens=1" %%a in ('arduino-cli.exe board list ^| findstr "Feather M0"') do set "port=%%a"

echo Detected Feather M0 port as: %port%

:loop
set /p sensor_id=Enter the sensor ID and hit enter to upload or type 'n' to cancel:
if "%sensor_id%"=="n" goto :eof

rem Modify the sketch file
echo Setting sensor ID to: %sensor_id%...
powershell -Command "(gc %sketch_path%) -replace 'int sensor_id = \d+;', 'int sensor_id = %sensor_id%;' | sc %sketch_path%"

rem Check if the modification was successful
powershell -Command "if ((gc %sketch_path%) -notmatch 'int sensor_id = %sensor_id%;') { exit 1 }"
if errorlevel 1 (
    echo Error: Failed to update sensor ID in %sketch_path%
    goto loop
)

rem Compile and upload
echo Compiling sketch...
arduino-cli.exe compile --fqbn %board% "%sketch_path%"
if errorlevel 1 (
    echo Compilation failed
    goto loop
)

echo Uploading to %port% with sensor ID %sensor_id%
arduino-cli.exe upload -p %port% --fqbn %board% "%sketch_path%"
if errorlevel 1 (
    echo Upload failed
    goto loop
)

echo Successfully uploaded to %port% with sensor ID %sensor_id%
echo.
echo CHECK SENSOR ID STICKER IS: %sensor_id%
echo.

goto loop
