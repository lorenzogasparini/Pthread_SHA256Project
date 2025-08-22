#!/bin/bash

# === CONFIG ===
SERVER_PATH=./build/server
CLIENT_PATH=./build/client
NUM_CLIENTS=20

# === Array of files to pass to clients ===
# 15 valid files, 5 invalid (non-existent)
FILES=(
    "./src/client.c"
    "./src/client.c"
    "./src/errExit.c"
    "./src/errExit.c"
    "./src/hashTable.c"
    "./src/hashTable.c"
    "./src/server.c"
    "./src/server.c"
    "./src/threadPool.c"
    "./src/threadPool.c"
    "./src/requestResponse.c"
    "./src/requestResponse.c"
    "./src/threadPool.c"
    "./src/hashTable.c"
    "./src/server.c"
    "./src/client"          # invalid (missing extension)
    "./src/server"          # invalid (missing extension)
    "./src/threadPool"      # invalid (missing extension)
    "./src/hashTable"       # invalid (missing extension)
    "./src/notExistingFile" # invalid (does not exist at all)
)

# === LAUNCH SERVER ===
echo "Starting server..."
$SERVER_PATH &   # run in background
SERVER_PID=$!

# === LAUNCH N CLIENTS ===
for ((i=0; i<$NUM_CLIENTS; i++)); do
    FILE=${FILES[$i]}
    echo "Starting client $((i+1)) with file: $FILE"
    $CLIENT_PATH "$FILE" &
done

# === WAIT ===
wait $SERVER_PID
echo "All processes finished."
