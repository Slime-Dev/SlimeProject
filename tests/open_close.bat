:: The executable dir is passed as an argument
:: this test will open the executable and let it run for 5 seconds before closing it

echo off

:: Check if the executable is passed as an argument

IF "%1"=="" (
    ECHO "Please pass the executable as an argument"
    EXIT /B 1
)

ECHO "Opening and closing the executable"

:: Output the executable path
ECHO "Executable path: %1"

:: Open the executable
START "" "%1"

:: Wait for 5 seconds
PING -n 6

:: Close the executable
TASKKILL /IM "%1" /F

:: if something goes wrong, exit with error code
IF ERRORLEVEL 1 (
    ECHO "Something went wrong"
    EXIT /B 1
)

:: Exit
EXIT /B 0
