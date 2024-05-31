# The executable dir is passed as an argument
# this test will open the executable and let it run for 5 seconds before closing it
# linux only

# Check if the executable is passed as an argument
if [ $# -ne 1 ]; then
    echo "Usage: $0 <executable>"
    exit 1
fi

# Check if the executable exists
if [ ! -f $1 ]; then
    echo "Executable not found"
    exit 1
fi

# list all files in the directory $0 
ls -l $0

# Open the executable
echo "Opening $1"
$1 &
pid=$!

# wait for the executable to start and if it fails return its exit code
wait $pid
if [ $? -ne 0 ]; then
    echo "Failed to open $1"
    exit 1
fi

# Wait for 5 seconds
echo "Running for 5 seconds"
sleep 5

# Get the process ID of the running executable
pid=$(pgrep $1)

# If the process ID is not empty, try to kill the process
if [ ! -z "$pid" ]; then
    echo "Closing $1"
    kill $pid
    wait $pid

    # Check if the executable is closed
    if [ $? -ne 0 ]; then
        echo "Failed to close $1"
        exit 1
    fi
else
    echo "$1 is not running"
fi

exit 0