#!/bin/bash

# News Aggregator - Development Stop Script
# This script stops all services

set -e

echo "Stopping News Aggregator Development Environment..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Stop C++ services
stop_service() {
    local service_name=$1
    local pid_file=$2
    
    if [ -f "$pid_file" ]; then
        local pid=$(cat "$pid_file")
        if kill -0 "$pid" 2>/dev/null; then
            print_status "Stopping $service_name (PID: $pid)..."
            kill "$pid"
            sleep 2
            if kill -0 "$pid" 2>/dev/null; then
                print_warning "Force killing $service_name..."
                kill -9 "$pid"
            fi
            print_success "$service_name stopped"
        else
            print_warning "$service_name was not running"
        fi
        rm -f "$pid_file"
    else
        print_warning "PID file for $service_name not found"
    fi
}

# Stop all C++ services
stop_service "StorageService" "logs/storage.pid"
stop_service "CollectorService" "logs/collector.pid"
stop_service "API Gateway" "logs/gateway.pid"

# Stop Docker services
print_status "Stopping Docker services..."
docker-compose down

print_success "All services stopped successfully!"

# Clean up any remaining processes
print_status "Cleaning up any remaining processes..."
pkill -f "news_aggregator_service" 2>/dev/null || true
pkill -f "collector_service" 2>/dev/null || true
pkill -f "api_gateway" 2>/dev/null || true

print_success "Cleanup completed!"
