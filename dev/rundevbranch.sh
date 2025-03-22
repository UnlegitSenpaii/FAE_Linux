#!/bin/bash
# Script to clone development branch, build, and test FAE_Linux

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color


TEMP_DIR="$(pwd)/temp"
OUTPUT_DIR="$(pwd)/out"

# Function to print colored messages
print_info() {
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

# Function to clean up on error
cleanup() {
    if [ -n "$TEMP_DIR" ] && [ -d "$TEMP_DIR" ]; then
        print_info "Cleaning up temporary directory..."
        rm -rf "$TEMP_DIR"
    fi
    exit 1
}

# Always clean up before exiting
cleanup_and_exit() {
    EXIT_CODE=$1
    if [ -n "$TEMP_DIR" ] && [ -d "$TEMP_DIR" ]; then
        print_info "Cleaning up temporary directory..."
        rm -rf "$TEMP_DIR"
    fi
    exit $EXIT_CODE
}

# Set trap to clean up on script exit
trap 'cleanup_and_exit $?' EXIT INT TERM

# Check if a factorio path was provided
if [ $# -lt 1 ]; then
    print_error "Usage: $0 <path_to_factorio_binary>"
    exit 1
fi

FACTORIO_PATH="$1"

# Check if the provided factorio path exists
if [ ! -f "$FACTORIO_PATH" ]; then
    print_error "Factorio binary not found at $FACTORIO_PATH"
    exit 1
fi


# Remove existing temp directory if it exists
if [ -d "$TEMP_DIR" ]; then
    print_info "Removing existing temp directory..."
    rm -rf "$TEMP_DIR"
fi

mkdir -p "$TEMP_DIR"
print_info "Created temporary directory: $TEMP_DIR"
 
# Check if the output directory exists, if not create it
if [ ! -d "$OUTPUT_DIR" ]; then
    print_info "Creating output directory: $OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"
else
    print_info "Output directory already exists: $OUTPUT_DIR"
fi

# Clone the development branch
print_info "Cloning development branch..."
git clone --depth 1 -b development https://github.com/UnlegitSenpaii/FAE_Linux.git "$TEMP_DIR/FAE_Linux" || {
    print_error "Failed to clone the repository"
    cleanup
}

# Navigate to the cloned repository
cd "$TEMP_DIR/FAE_Linux" || {
    print_error "Failed to navigate to the cloned repository"
    cleanup
}

# Build the project
print_info "Building project..."
cmake CMakeLists.txt || {
    print_error "Failed to run cmake"
    cleanup
}

make || {
    print_error "Failed to build the project"
    cleanup
}

# Create a timestamp for the patched file
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
PATCHED_BINARY="factorio_patched_$TIMESTAMP"

# Copy the factorio binary
print_info "Copying Factorio binary..."
cp "$FACTORIO_PATH" "$TEMP_DIR/$PATCHED_BINARY" || {
    print_error "Failed to copy the Factorio binary"
    cleanup
}

# Run the patcher
print_info "Running the patcher..."
"$TEMP_DIR/FAE_Linux/out/bin/FAE_Linux" "$TEMP_DIR/$PATCHED_BINARY"
PATCHER_EXIT_CODE=$?

# Move only the patched binary to the output directory if successful
if [ $PATCHER_EXIT_CODE -eq 0 ]; then
    mv "$TEMP_DIR/$PATCHED_BINARY" "$OUTPUT_DIR/$PATCHED_BINARY" || {
        print_error "Failed to move patched binary to output directory"
        print_info "Patched binary is still available at: $TEMP_DIR/$PATCHED_BINARY"
        exit 1
    }
    print_success "Patching completed successfully!"
    print_info "Patched binary is available at: $OUTPUT_DIR/$PATCHED_BINARY"
else
    print_error "Patcher exited with error code: $PATCHER_EXIT_CODE"
fi

# Script will automatically clean up temp directory due to the trap
exit $PATCHER_EXIT_CODE