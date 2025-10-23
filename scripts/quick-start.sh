#!/bin/bash

# News Aggregator - Quick Start Script
# This script builds and starts everything with one command

set -e

echo "News Aggregator - Quick Start"
echo "=============================="

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

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "Please run this script from the project root directory"
    exit 1
fi

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    print_error "Docker is not running. Please start Docker first."
    exit 1
fi

# Check if docker-compose is available
if ! command -v docker-compose &> /dev/null; then
    print_error "docker-compose is not installed. Please install docker-compose first."
    exit 1
fi

echo ""
print_status "Step 1: Building the project..."
./scripts/build.sh

echo ""
print_status "Step 2: Starting all services..."
./scripts/start-dev.sh

echo ""
print_success "News Aggregator is ready!"
echo ""
echo "Quick Commands:"
echo "  View news: curl http://localhost:8083/news"
echo "  Health check: curl http://localhost:8083/health"
echo "  Stop all: ./scripts/stop-dev.sh"
echo "  View logs: tail -f logs/*.log"
echo ""
echo "Open in browser:"
echo "  API Gateway: http://localhost:8083"
echo "  StorageService: http://localhost:8080"
echo "  CollectorService: http://localhost:8081"
