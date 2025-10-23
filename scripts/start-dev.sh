#!/bin/bash

# News Aggregator - Development Startup Script
# This script starts all services for development

set -e

echo "Starting News Aggregator Development Environment..."

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

# Check if build directory exists
if [ ! -d "build" ]; then
    print_error "Build directory not found. Please run 'mkdir build && cd build && cmake .. && make' first"
    exit 1
fi

# Check if executables exist
if [ ! -f "build/news_aggregator_service" ] || [ ! -f "build/collector_service" ] || [ ! -f "build/api_gateway" ]; then
    print_error "Executables not found. Please build the project first"
    exit 1
fi

# Start Docker services
print_status "Starting Docker services (PostgreSQL, Redis)..."
docker-compose up -d

# Wait for PostgreSQL to be ready
print_status "Waiting for PostgreSQL to be ready..."
sleep 5

# Check if PostgreSQL is ready
max_attempts=30
attempt=1
while [ $attempt -le $max_attempts ]; do
    if docker-compose exec -T postgres pg_isready -U news_user -d news_db > /dev/null 2>&1; then
        print_success "PostgreSQL is ready!"
        break
    fi
    print_status "Waiting for PostgreSQL... (attempt $attempt/$max_attempts)"
    sleep 2
    attempt=$((attempt + 1))
done

if [ $attempt -gt $max_attempts ]; then
    print_error "PostgreSQL failed to start within expected time"
    exit 1
fi

# Initialize database
print_status "Initializing database..."
if [ -f "sql/001_create_news_table.sql" ]; then
    docker-compose exec -T postgres psql -U news_user -d news_db -f /docker-entrypoint-initdb.d/001_create_news_table.sql
    print_success "Database initialized"
else
    print_warning "Database initialization script not found, skipping..."
fi

# Create logs directory
mkdir -p logs

# Start services in background
print_status "Starting StorageService..."
nohup ./build/news_aggregator_service --config configs/static_config.yaml > logs/storage.log 2>&1 &
STORAGE_PID=$!
echo $STORAGE_PID > logs/storage.pid

# Wait a bit for StorageService to start
sleep 3

print_status "Starting CollectorService..."
nohup ./build/collector_service --config configs/collector_config.yaml > logs/collector.log 2>&1 &
COLLECTOR_PID=$!
echo $COLLECTOR_PID > logs/collector.pid

print_status "Starting API Gateway..."
nohup ./build/api_gateway --config configs/gateway_config.yaml > logs/gateway.log 2>&1 &
GATEWAY_PID=$!
echo $GATEWAY_PID > logs/gateway.pid

# Wait for services to start
print_status "Waiting for services to start..."
sleep 5

# Check if services are running
check_service() {
    local service_name=$1
    local port=$2
    local pid_file=$3
    
    if [ -f "$pid_file" ] && kill -0 $(cat "$pid_file") 2>/dev/null; then
        if curl -s "http://localhost:$port" > /dev/null 2>&1; then
            print_success "$service_name is running on port $port"
            return 0
        fi
    fi
    print_error "$service_name failed to start"
    return 1
}

# Check all services
services_ok=true
check_service "StorageService" "8080" "logs/storage.pid" || services_ok=false
check_service "CollectorService" "8081" "logs/collector.pid" || services_ok=false
check_service "API Gateway" "8083" "logs/gateway.pid" || services_ok=false

if [ "$services_ok" = true ]; then
    print_success "All services started successfully!"
    echo ""
    echo "Service URLs:"
    echo "  StorageService: http://localhost:8080"
    echo "  CollectorService: http://localhost:8081"
    echo "  API Gateway: http://localhost:8083"
    echo ""
    echo "Test the API:"
    echo "  curl http://localhost:8083/news"
    echo ""
    echo "View logs:"
    echo "  tail -f logs/storage.log"
    echo "  tail -f logs/collector.log"
    echo "  tail -f logs/gateway.log"
    echo ""
    echo "Stop services:"
    echo "  ./scripts/stop-dev.sh"
else
    print_error "Some services failed to start. Check the logs for details."
    exit 1
fi
