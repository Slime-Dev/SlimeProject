# The executable dir is passed as an argument
# this test will open the executable and let it run for 5 seconds before closing it

# Check if the executable is passed as an argument
if [ $# -ne 1 ]; then
    echo "Usage: $0 <executable>"
    exit 1
fi

# Check if the executable exists
if [ ! -f $1 ]; then
    echo "Executable $1 does not exist"
    exit 1
fi

# Open the executable
$1 &
pid=$!
echo "Executable $1 is running with PID $pid"

# Wait for 5 seconds
sleep 5

# Close the executable
echo "Closing executable $1"
kill -9 $pid
exit 0

