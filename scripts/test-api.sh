#!/bin/bash

# News Aggregator API Test Script
# Tests all API endpoints and shows formatted output

set -e

API_BASE="http://localhost:8083"

echo "News Aggregator API Test"
echo "========================"
echo ""

# Test health endpoint
echo "1. Testing Health Endpoint:"
echo "---------------------------"
curl -s "${API_BASE}/health" | python3 -m json.tool
echo ""

# Test news endpoint with different limits
echo "2. Testing News Endpoint (limit=3):"
echo "-----------------------------------"
curl -s "${API_BASE}/news?limit=3" | python3 -m json.tool
echo ""

# Test news endpoint with limit=1
echo "3. Testing News Endpoint (limit=1):"
echo "-----------------------------------"
curl -s "${API_BASE}/news?limit=1" | python3 -m json.tool
echo ""

# Test ping endpoint
echo "4. Testing Ping Endpoint:"
echo "-------------------------"
curl -s "${API_BASE}/ping"
echo ""
echo ""

echo "All tests completed!"
echo ""
echo "Available endpoints:"
echo "  Health: ${API_BASE}/health"
echo "  News:   ${API_BASE}/news?limit=N"
echo "  Ping:   ${API_BASE}/ping"
echo ""
echo "For formatted JSON, use:"
echo "  curl -s '${API_BASE}/news?limit=3' | python3 -m json.tool"
echo "  python3 test_api.py --limit 3"
echo "  ./scripts/format-json.sh news 3"
