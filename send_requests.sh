#!/bin/bash

# Define the URL of your web server
# URL="http://localhost:8080"  # Change this to your server's URL
URL="http://localhost:8080/ui-test"  # Change this to your server's URL

# Define the number of simultaneous requests
NUM_REQUESTS=1000

# Function to send a single request in background
send_request() {
    curl --tlsv1.2 -s -o /dev/null "$URL" > /dev/null 2>&1
}

# Send multiple requests simultaneously
for ((i=0; i<NUM_REQUESTS; i++)); do
    send_request &
done

# Wait for all background jobs to finish
wait
