#!/data/data/com.termux/files/usr/bin/bash
set -e
echo "=== MimiClaw Termux Setup ==="
pkg update -y
pkg install -y git make gcc curl sqlite libcurl

# Clone if not already present
if [ ! -d ~/mimiclaw ]; then
    git clone https://github.com/user/mimiclaw.git ~/mimiclaw
fi

cd ~/mimiclaw

# Setup test dependencies
cd test
make setup

# Build core
cd ../core
make setup
make

echo "=== Setup complete ==="
echo "Tests: cd ~/mimiclaw/test && make test"
echo "Run:   cd ~/mimiclaw/core && ./mimiclaw"
