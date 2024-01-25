#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <output_directory>"
    exit 1
fi

output_directory="$1"

# Ensure the output directory ends with a slash '/'
if [ "${output_directory: -1}" != "/" ]; then
    echo "Error: The output directory must end with a slash '/'"
    exit 1
fi

timestamp=$(date +"%Y%m%d%H%M%S")
filename="${output_directory}${timestamp}.sql"

# Create the migration file
touch "$filename"

echo "File created: $filename"
