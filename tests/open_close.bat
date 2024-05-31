:: The executable dir is passed as an argument
:: this test will open the executable and let it run for 5 seconds before closing it
:: Must not use input redirection or output redirection as it will not work in windows

@ECHO OFF
SET EXECUTABLE_PATH=%~1

:: Get just the executable name eg. "SlimeOdyssey.exe"
SET EXECUTABLE_NAME=%~nx1

IF "%EXECUTABLE_PATH%"=="" (
    ECHO "Please provide the executable path"
    EXIT /B 1
)

START /wait "" "%EXECUTABLE_PATH%"
IF ERRORLEVEL 1 (
    ECHO "Failed to start the executable"
    EXIT /B 1
)

:: Wait for 5 seconds
PING -n 6

:: Close the executable
TASKKILL /IM "%EXECUTABLE_NAME%" /F

EXIT /B 0