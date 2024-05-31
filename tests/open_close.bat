:: The executable dir is passed as an argument
:: this test will open the executable and let it run for 5 seconds before closing it
:: Must not use input redirection or output redirection as it will not work in windows

@ECHO OFF
SET EXECUTABLE_PATH=%~1
IF "%EXECUTABLE_PATH%"=="" (
    ECHO "Please provide the executable path"
    EXIT /B 1
)

:: Use WMIC to start and get the process id of the executable in one line
FOR /F "tokens=2 delims=," %%A IN ('WMIC PROCESS CALL CREATE "%EXECUTABLE_PATH%" ^| FIND "ProcessId"') DO SET PID=%%A

:: Wait for 5 seconds
PING 
TIMEOUT /T 5

:: Use WMIC to kill the process
WMIC PROCESS WHERE "ProcessId=%PID%" DELETE

EXIT /B 0