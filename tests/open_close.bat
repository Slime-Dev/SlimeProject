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

    :: Start the executable and grab its PID
    START "" "%EXECUTABLE_PATH%"
    SET PID=
    FOR /F "tokens=2" %%A IN ('TASKLIST /FI "IMAGENAME eq %EXECUTABLE_PATH%" /FO LIST ^| FIND "PID:"') DO SET PID=%%A

    :: Wait for 5 seconds
    TIMEOUT /T 5 /NOBREAK

    :: Close the executable
    IF DEFINED PID (
        TASKKILL /F /PID %PID%
    ) ELSE (
        ECHO Failed to find PID for executable
    )
) ELSE (
    ECHO Executable not found: %EXECUTABLE_PATH%
)

ENDLOCAL

:: Exit with the error code of the executable
EXIT /B %ERRORLEVEL%
