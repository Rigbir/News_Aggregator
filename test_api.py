#!/usr/bin/env python3
"""
News Aggregator API Test Client
Usage: python3 test_api.py [--limit N] [--endpoint ENDPOINT]
"""

import json
import urllib.request
import urllib.parse
import argparse
import sys
from typing import Dict, Any

API_BASE = "http://localhost:8083"

def format_json(data: Dict[Any, Any], indent: int = 2) -> str:
    """Format JSON with proper indentation"""
    return json.dumps(data, indent=indent, ensure_ascii=False)

def fetch_data(endpoint: str, limit: int = 5) -> Dict[Any, Any]:
    """Fetch data from API endpoint"""
    url = f"{API_BASE}/{endpoint}"
    if endpoint == "news":
        url += f"?limit={limit}"
    
    try:
        with urllib.request.urlopen(url, timeout=10) as response:
            data = response.read().decode('utf-8')
            return json.loads(data)
    except Exception as e:
        print(f"Error fetching data: {e}")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description="Test News Aggregator API")
    parser.add_argument("--limit", type=int, default=5, help="Number of news items to fetch")
    parser.add_argument("--endpoint", default="news", help="API endpoint (news, health)")
    parser.add_argument("--raw", action="store_true", help="Show raw JSON without formatting")
    
    args = parser.parse_args()
    
    print(f"Fetching data from {API_BASE}/{args.endpoint}...")
    if args.endpoint == "news":
        print(f"Limit: {args.limit}")
    print()
    
    try:
        data = fetch_data(args.endpoint, args.limit)
        
        if args.raw:
            print(json.dumps(data, ensure_ascii=False))
        else:
            print("Formatted JSON:")
            print("=" * 50)
            print(format_json(data))
            print("=" * 50)
            
            # Show summary for news endpoint
            if args.endpoint == "news" and isinstance(data, dict):
                news_count = data.get("count", 0)
                status = data.get("status", "unknown")
                cache_status = data.get("cache_status", "unknown")
                
                print(f"\nSummary:")
                print(f"   Status: {status}")
                print(f"   News count: {news_count}")
                print(f"   Cache: {cache_status}")
                
    except KeyboardInterrupt:
        print("\nGoodbye!")
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
