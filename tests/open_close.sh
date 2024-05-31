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

# Open the executable
echo "Opening $1"
$1 &
pid=$!

# Wait for 5 seconds
echo "Running for 5 seconds"
sleep 5

# Close the executable
echo "Closing $1"
kill $pid
wait $pid

exit 0