:: The executable dir is passed as an argument
:: this test will open the executable and let it run for 5 seconds before closing it

@ECHO OFF
SET EXECUTABLE_PATH=%~1
SET EXECUTABLE_PATH=%EXECUTABLE_PATH:/=\%

ECHO Opening executable at %EXECUTABLE_PATH%
START "" "%EXECUTABLE_PATH%"
TIMEOUT /T 5
TASKKILL /IM %EXECUTABLE_PATH% /F
ECHO Closed executable at %EXECUTABLE_PATH%
