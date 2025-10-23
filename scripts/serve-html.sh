#!/bin/bash

# Simple HTTP server for serving HTML client
# This avoids CORS issues when opening HTML files directly

PORT=${1:-8084}
HTML_FILE="test_client.html"

echo "Starting HTTP server for HTML client..."
echo "Port: $PORT"
echo "File: $HTML_FILE"
echo ""
echo "Open in browser: http://localhost:$PORT/$HTML_FILE"
echo "Press Ctrl+C to stop"
echo ""

# Check if Python 3 is available
if command -v python3 &> /dev/null; then
    echo "Using Python 3 HTTP server..."
    python3 -m http.server $PORT
elif command -v python &> /dev/null; then
    echo "Using Python HTTP server..."
    python -m SimpleHTTPServer $PORT
else
    echo "Error: Python not found. Please install Python to use this script."
    exit 1
fi
