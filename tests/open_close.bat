:: The executable dir is passed as an argument
:: this test will open the executable and let it run for 5 seconds before closing it

:: Check if the executable is passed as an argument

IF "%1"=="" (
    ECHO "Please pass the executable as an argument"
    EXIT /B 1
)

:: Open the executable
START "" "%1"

:: Wait for 5 seconds
PING -n 6

:: Close the executable
TASKKILL /IM "%1" /F

:: Exit
EXIT /B 0
