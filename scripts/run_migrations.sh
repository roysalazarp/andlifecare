#!/bin/bash

if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <migration-folder> <db-host> <db-port> <db-name> <db-user>"
    exit 1
fi

MIGRATION_FOLDER="$1"
DB_HOST="$2"
DB_PORT="$3"
DB_NAME="$4"
DB_USER="$5"

# Check if the migration folder exists
if [ ! -d "$MIGRATION_FOLDER" ]; then
    echo "Error: Migration folder not found."
    exit 1
fi

# Change to the migration folder
cd "$MIGRATION_FOLDER" || exit

# Find and sort SQL migration files
migration_files=$(find . -type f -name "*.sql" | sort)

# Loop through and run each migration
for migration_file in $migration_files; do
    echo "Running migration: $migration_file"
    psql -h $DB_HOST -p $DB_PORT -d $DB_NAME -U $DB_USER -f $migration_file

    if [ $? -ne 0 ]; then
        echo "Error running migration. Exiting."
        exit 1
    fi
done

echo "All migrations successfully applied."
