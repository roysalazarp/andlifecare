#!/bin/bash

: "
+-----------------------------------------------------------------------------------+
|   This script creates a new blank migration file with the current date and time   | 
|   as the filename.                                                                |
+-----------------------------------------------------------------------------------+
                                                                \   ^__^
                                                                 \  (oo)\_______
                                                                    (__)\       )\/\
                                                                        ||----w |
                                                                        ||     ||                           
"

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <output_directory>"
    exit 1
fi

OUTPUT_DIRECTORY="$1"

# Ensure the output directory ends with a slash '/'
if [ "${OUTPUT_DIRECTORY: -1}" != "/" ]; then
    echo "Error: The output directory must end with a slash '/'"
    exit 1
fi

timestamp=$(date +"%Y%m%d%H%M%S")
filename="${OUTPUT_DIRECTORY}${timestamp}.sql"

touch "$filename"

echo "File created: $filename"
