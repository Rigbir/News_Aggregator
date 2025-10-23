#!/bin/bash

# News Aggregator JSON Formatter
# Usage: ./scripts/format-json.sh [endpoint] [limit]

set -e

API_BASE="http://localhost:8083"
ENDPOINT="${1:-news}"
LIMIT="${2:-5}"

echo "Fetching data from ${API_BASE}/${ENDPOINT}?limit=${LIMIT}..."
echo ""

# Check if jq is available
if command -v jq &> /dev/null; then
    echo "Using jq for formatting:"
    echo "----------------------------------------"
    curl -s "${API_BASE}/${ENDPOINT}?limit=${LIMIT}" | jq '.'
elif command -v python3 &> /dev/null; then
    echo "Using python3 for formatting:"
    echo "----------------------------------------"
    curl -s "${API_BASE}/${ENDPOINT}?limit=${LIMIT}" | python3 -m json.tool
else
    echo "Neither jq nor python3 found. Install one of them:"
    echo "   brew install jq"
    echo "   # or python3 is usually pre-installed"
    echo ""
    echo "Raw JSON output:"
    echo "----------------------------------------"
    curl -s "${API_BASE}/${ENDPOINT}?limit=${LIMIT}"
fi

echo ""
echo "Done!"
