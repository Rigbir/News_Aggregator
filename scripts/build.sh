#!/bin/bash

# News Aggregator - Build Script
# This script builds the project

set -e

echo "Building News Aggregator..."

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

# Check if cmake is available
if ! command -v cmake &> /dev/null; then
    print_error "CMake is not installed. Please install CMake first."
    exit 1
fi

# Check if make is available
if ! command -v make &> /dev/null; then
    print_error "Make is not installed. Please install Make first."
    exit 1
fi

# Create build directory
print_status "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
print_status "Configuring with CMake..."
cmake ..

# Build the project
print_status "Building the project..."
make -j$(nproc 2>/dev/null || echo 4)

# Check if executables were created
print_status "Checking if executables were created..."
if [ -f "news_aggregator_service" ] && [ -f "collector_service" ] && [ -f "api_gateway" ]; then
    print_success "Build completed successfully!"
    echo ""
    echo "Built executables:"
    echo "  news_aggregator_service"
    echo "  collector_service"
    echo "  api_gateway"
    echo ""
    echo "To start the development environment:"
    echo "  ./scripts/start-dev.sh"
else
    print_error "Build failed - some executables are missing"
    exit 1
fi
