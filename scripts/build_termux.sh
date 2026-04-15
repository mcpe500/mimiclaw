#!/data/data/com.termux/files/usr/bin/bash
set -e
echo "=== MimiClaw Termux Build ==="

# Install dependencies
pkg install -y git make gcc curl sqlite libcurl

# Build core runtime
cd "$(dirname "$0")/../core"
make clean
make setup
make

echo "=== Build complete: ./mimiclaw ==="
echo "Run: ./mimiclaw"
