#!/bin/bash

# News Aggregator - Status Check Script
# This script checks the status of all services

set -e

echo "News Aggregator - Service Status"
echo "================================="

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

# Check if service is running
check_service() {
    local service_name=$1
    local port=$2
    local pid_file=$3
    
    echo -n "Checking $service_name... "
    
    if [ -f "$pid_file" ]; then
        local pid=$(cat "$pid_file")
        if kill -0 "$pid" 2>/dev/null; then
            if curl -s "http://localhost:$port" > /dev/null 2>&1; then
                print_success "Running (PID: $pid, Port: $port)"
                return 0
            else
                print_warning "Process running but not responding on port $port"
                return 1
            fi
        else
            print_error "Process not running (stale PID file)"
            return 1
        fi
    else
        print_error "Not running (no PID file)"
        return 1
    fi
}

# Check Docker services
check_docker() {
    echo -n "Checking Docker services... "
    if docker-compose ps | grep -q "Up"; then
        print_success "Running"
        echo "  Docker containers:"
        docker-compose ps --format "table" | grep -v "NAME"
    else
        print_error "Not running"
        return 1
    fi
}

echo ""
print_status "Checking Docker services..."
check_docker

echo ""
print_status "Checking C++ services..."

# Check all C++ services
services_ok=true
check_service "StorageService" "8080" "logs/storage.pid" || services_ok=false
check_service "CollectorService" "8081" "logs/collector.pid" || services_ok=false
check_service "API Gateway" "8083" "logs/gateway.pid" || services_ok=false

echo ""
if [ "$services_ok" = true ]; then
    print_success "All services are running!"
    echo ""
    echo "Service URLs:"
    echo "  StorageService: http://localhost:8080"
    echo "  CollectorService: http://localhost:8081"
    echo "  API Gateway: http://localhost:8083"
    echo ""
    echo "Test the API:"
    echo "  curl http://localhost:8083/news"
else
    print_warning "Some services are not running properly."
    echo ""
    echo "To start all services:"
    echo "  ./scripts/start-dev.sh"
    echo ""
    echo "To view logs:"
    echo "  tail -f logs/*.log"
fi
