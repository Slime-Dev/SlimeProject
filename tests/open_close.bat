:: The executable dir is passed as an argument
:: this test will open the executable and let it run for 5 seconds before closing it

@ECHO OFF
SETLOCAL

:: Convert forward slashes to backslashes for compatibility
SET EXECUTABLE_PATH=%~1
SET EXECUTABLE_PATH=%EXECUTABLE_PATH:/=\%

:: Check if the executable exists
IF EXIST "%EXECUTABLE_PATH%" (
    ECHO Starting executable: %EXECUTABLE_PATH%
    START "" "%EXECUTABLE_PATH%"
    
    :: Wait for 5 seconds
    PING -n 6 > NUL
    
    :: Attempt to close the executable
    TASKKILL /IM "%EXECUTABLE_PATH%" /F
) ELSE (
    ECHO Executable not found: %EXECUTABLE_PATH%
    EXIT /B 1
)

ENDLOCAL
EXIT /B 0
