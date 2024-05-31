:: The executable dir is passed as an argument
:: this test will open the executable and let it run for 5 seconds before closing it
:: Must not use input redirection or output redirection as it will not work in windows

SET EXECUTABLE_PATH=%~1
IF "%EXECUTABLE_PATH%"=="" (
    ECHO "Please provide the executable path"
    EXIT /B 1
)

:: Check if the executable exists
IF NOT EXIST "%EXECUTABLE_PATH%" (
    ECHO "The executable does not exist at the provided path"
    EXIT /B 1
)

:: Try to start the process and check if it was successful
FOR /F "tokens=2 delims=;=" %%A IN ('WMIC PROCESS CALL CREATE "%EXECUTABLE_PATH%" ^| FIND "ProcessId"') DO SET PID=%%A
IF "%PID%"=="" (
    ECHO "Failed to start the process"
    EXIT /B 1
)

:: Wait for 5 seconds
TIMEOUT /T 5

:: Try to kill the process and check if it was successful
WMIC PROCESS WHERE "ProcessId=%PID%" DELETE
IF ERRORLEVEL 1 (
    ECHO "Failed to kill the process"
    EXIT /B 1
)

EXIT /B 0