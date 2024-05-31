:: The executable dir is passed as an argument
:: this test will open the executable and let it run for 5 seconds before closing it
:: Must not use input redirection or output redirection as it will not work in windows

SET EXECUTABLE_PATH=%~1
IF "%EXECUTABLE_PATH%"=="" (
    ECHO "Please provide the executable path"
    EXIT /B 1
)

WMIC PROCESS CALL CREATE "%EXECUTABLE_PATH%"

:: Wait for 5 seconds
TIMEOUT /T 5

:: Use WMIC to kill the process
WMIC PROCESS WHERE "CommandLine LIKE '%%%EXECUTABLE_PATH%%%'" DELETE

:: Exit with the exit code of the executable
EXIT /B %ERRORLEVEL%