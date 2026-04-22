#!/bin/bash

#
# @file run_server.sh
# @author Roman Snitko
# @date 2026-04-19
# @brief Scrip to run web_htop server with optional parameters
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
SERVER_BIN="${BUILD_DIR}/server/web_htop_server"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Print error and exit
error() {
    echo -e "${RED}Error: $@${NC}" >&2
    exit 1
}

# Print info message
info() {
    echo -e "${GREEN}[INFO]${NC} $@"
}

# Print warning message
warning() {
    echo -e "${YELLOW}[WARN]${NC} $@"
}

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    error "Build directory not found at $BUILD_DIR"
fi

# Check if server binary exists
if [ ! -f "$SERVER_BIN" ]; then
    error "Server binary not found at $SERVER_BIN"
fi

# Check if server binary is executable
if [ ! -x "$SERVER_BIN" ]; then
    warning "Server binary is not executable, attempting to fix permissions..."
    chmod +x "$SERVER_BIN" || error "Failed to make server executable"
fi

info "Starting Web Htop Server..."
info "Location: $SERVER_BIN"
info "To stop the server, press Ctrl+C"
echo ""

# Run the server
"$SERVER_BIN" "$@"

EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    info "Server stopped successfully"
else
    error "Server exited with code $EXIT_CODE"
fi

exit $EXIT_CODE
