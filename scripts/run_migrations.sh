#!/bin/bash

# Check if required arguments are provided
if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <migration_folder> <host> <database> <user>"
    exit 1
fi

# Assign command-line arguments to variables
MIGRATION_FOLDER="$1"
HOST="$2"
DATABASE="$3"
USER="$4"

# Check if the migration folder exists
if [ ! -d "$MIGRATION_FOLDER" ]; then
    echo "Error: Migration folder not found."
    exit 1
fi

# Change to the migration folder
cd "$MIGRATION_FOLDER" || exit

# Find and sort SQL migration files
MIGRATION_FILES=$(find . -type f -name "*.sql" | sort)

# Loop through and run each migration
for FILE in $MIGRATION_FILES; do
    echo "Running migration: $FILE"
    psql -h "$HOST" -d "$DATABASE" -U "$USER" -f "$FILE"
    # Add any additional connection parameters (-h, -d, -U) as needed
    # Replace "$HOST", "$DATABASE", and "$USER" with your actual database connection details
    if [ $? -ne 0 ]; then
        echo "Error running migration. Exiting."
        exit 1
    fi
done

echo "All migrations successfully applied."
