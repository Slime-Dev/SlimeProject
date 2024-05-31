:: The executable dir is passed as an argument
:: this test will open the executable and let it run for 5 seconds before closing it
:: Must not use input redirection or output redirection as it will not work in windows

@ECHO OFF
SET EXECUTABLE_PATH=%~1
IF "%EXECUTABLE_PATH%"=="" (
    ECHO "Please provide the executable path"
    EXIT /B 1
)

:: Use WMIC to start and get the process id of the executable
WMIC PROCESS CALL CREATE "%EXECUTABLE_PATH%"
IF ERRORLEVEL 1 (
    ECHO "Failed to start the executable"
    EXIT /B 1
)

SET PID=
FOR /F "tokens=2 delims=," %%A IN ('WMIC PROCESS WHERE "CommandLine LIKE '%%%EXECUTABLE_PATH%%%'" GET ProcessId /VALUE') DO (
    SET PID=%%A
)

:: Wait for 5 seconds
PING -n 6

:: Close the executable
TASKKILL /PID %PID% /F
IF ERRORLEVEL 1 (
    ECHO "Failed to close the executable"
    EXIT /B 1
)

EXIT /B 0