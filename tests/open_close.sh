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
$1 &
pid=$!

# Wait for 5 seconds
sleep 5

# Close the executable
kill $pid
wait $pid

exit 0