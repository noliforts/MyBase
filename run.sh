#!/bin/bash
set -e

if [ ! -f "build/db_server" ]; then
    echo "Server executable not found. Running build.sh..."
    ./build.sh
fi

HOST="127.0.0.1"
PORT="9000"

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --host) HOST="$2"; shift ;;
        --port) PORT="$2"; shift ;;
        *) echo "Unknown parameter: $1"; exit 1 ;;
    esac
    shift
done

echo "=> Starting server on $HOST:$PORT..."
./build/db_server --host "$HOST" --port "$PORT"
