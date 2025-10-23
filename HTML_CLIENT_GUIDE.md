# HTML Client Guide

## CORS Issue

When you open an HTML file directly in the browser (`file://`), the browser blocks requests to `http://localhost` due to CORS (Cross-Origin Resource Sharing) policy.

## Solutions

### 1. Use HTTP Server (Recommended)

Start a simple HTTP server for the HTML client:

```bash
# Start HTTP server on port 8084
./scripts/serve-html.sh 8084

# Open in browser:
# http://localhost:8084/test_client.html
```

### 2. Use Python Script

```bash
# Simple API testing
python3 test_api.py --limit 5

# Or with formatting
python3 test_api.py --limit 3
```

### 3. Use Bash Script

```bash
# Formatted output
./scripts/format-json.sh news 5

# Or comprehensive testing
./scripts/test-api.sh
```

### 4. Direct curl

```bash
# With formatting
curl -s "http://localhost:8083/news?limit=3" | python3 -m json.tool

# With jq (if installed)
curl -s "http://localhost:8083/news?limit=3" | jq '.'
```

## What's Fixed

1. **CORS headers** added to API Gateway
2. **HTTP server** created for HTML client
3. **Improved error handling** in HTML client
4. **Multiple API testing methods**

## Verification

1. Make sure all services are running:
   ```bash
   ./scripts/start-dev.sh
   ```

2. Check API:
   ```bash
   curl http://localhost:8083/health
   ```

3. Start HTML client:
   ```bash
   ./scripts/serve-html.sh 8084
   # Open http://localhost:8084/test_client.html
   ```

## Alternatives

If the HTML client still doesn't work, use:

- **Python script**: `python3 test_api.py`
- **Bash script**: `./scripts/format-json.sh news 5`
- **Comprehensive testing**: `./scripts/test-api.sh`