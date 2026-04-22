#!/bin/bash

#
# @file format.sh
# @author Roman Snitko
# @date 2026-04-19
# @brief Format source code using clang-format
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

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

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    warning "clang-format is not installed"
    echo "Install it with: sudo apt-get install clang-format"
    exit 1
fi

info "Clang-format version: $(clang-format --version)"
echo ""
info "Formatting C++ source files in $PROJECT_ROOT..."
echo ""

# Find and format all C++ files
FORMATTED_FILES=0
FAILED_FILES=0

# Format header files
for file in $(find "$PROJECT_ROOT" -name "*.hpp" -o -name "*.h" | grep -v build | grep -v ".git"); do
    if [ -f "$file" ]; then
        info "Formatting: $file"
        if clang-format -i "$file"; then
            ((FORMATTED_FILES++))
        else
            ((FAILED_FILES++))
            warning "Failed to format: $file"
        fi
    fi
done

# Format source files
for file in $(find "$PROJECT_ROOT" -name "*.cpp" | grep -v build | grep -v ".git"); do
    if [ -f "$file" ]; then
        info "Formatting: $file"
        if clang-format -i "$file"; then
            ((FORMATTED_FILES++))
        else
            ((FAILED_FILES++))
            warning "Failed to format: $file"
        fi
    fi
done

echo ""
info "Formatting complete!"
info "Formatted files: $FORMATTED_FILES"

if [ $FAILED_FILES -gt 0 ]; then
    warning "Failed to format $FAILED_FILES files"
    exit 1
fi

exit 0
