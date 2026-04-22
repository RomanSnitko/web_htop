#!/bin/bash

#
# @file run_client.sh
# @author Roman Snitko
# @date 2026-04-19
# @brief Script to run web_htop client
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
CLIENT_BIN="${BUILD_DIR}/client/web_htop_client"

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

# Print usage
usage() {
    echo "Usage: $0 <host> <streaming_port> [http_port]"
    echo ""
    echo "Arguments:"
    echo "  host            Server hostname or IP address (e.g., localhost, 127.0.0.1)"
    echo "  streaming_port  Streaming TCP server port (e.g., 9999)"
    echo "  http_port       HTTP API server port (default: 8080)"
    echo ""
    echo "Examples:"
    echo "  $0 localhost 9999"
    echo "  $0 localhost 9999 8080"
    echo "  $0 192.168.1.100 9999 8080"
}

# Check arguments
if [ $# -lt 2 ]; then
    error "Missing arguments. $0 requires <host> and <streaming_port>"
    usage
    exit 1
fi

HOST="$1"
STREAMING_PORT="$2"
HTTP_PORT="${3:-8080}"

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    error "Build directory not found at $BUILD_DIR"
fi

# Check if client binary exists
if [ ! -f "$CLIENT_BIN" ]; then
    error "Client binary not found at $CLIENT_BIN"
fi

# Check if client binary is executable
if [ ! -x "$CLIENT_BIN" ]; then
    error "Client binary is not executable"
fi

info "Starting Web Htop Client..."
info "Location: $CLIENT_BIN"
info "Streaming endpoint: $HOST:$STREAMING_PORT"
info "HTTP endpoint: $HOST:$HTTP_PORT"
echo ""

# Run the client
"$CLIENT_BIN" "$HOST" "$STREAMING_PORT" "$HTTP_PORT"

EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    info "Client stopped successfully"
else
    error "Client exited with code $EXIT_CODE"
fi

exit $EXIT_CODE
