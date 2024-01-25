#!/bin/bash

# Parse command-line arguments
if [ "$#" -ne 5 ]; then
    echo "Usage: $0 <db-host> <db-port> <db-name> <db-user> <mock-data-file>"
    exit 1
fi

# Database connection parameters
DB_HOST="$1"
DB_PORT="$2"
DB_NAME="$3"
DB_USER="$4"
MOCK_DATA_FILE="$5"

# Function to execute SQL commands
execute_sql() {
    psql -h $DB_HOST -p $DB_PORT -d $DB_NAME -U $DB_USER -f "$1"
}

# Execute SQL commands from the mock data file
execute_sql "$MOCK_DATA_FILE"

echo "Data inserted successfully."
