:: The executable dir is passed as an argument
:: this test will open the executable and let it run for 5 seconds before closing it

@ECHO OFF
SETLOCAL

:: Convert forward slashes to backslashes for compatibility
SET EXECUTABLE_PATH=%~1
SET EXECUTABLE_PATH=%EXECUTABLE_PATH:/=\%

:: Check if the executable exists
IF NOT EXIST "%EXECUTABLE_PATH%" (
    ECHO The executable does not exist
    EXIT /B 1
)

:: Open the executable
START "" "%EXECUTABLE_PATH%"
:: Wait for 5 seconds
TIMEOUT /T 5 /NOBREAK
:: Close the executable
TASKKILL /F /IM "%EXECUTABLE_PATH%" /T

ENDLOCAL

:: Exit with the error code of the executable
EXIT /B %ERRORLEVEL%
