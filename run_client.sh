#!/usr/bin/env bash
# Run the Dreadmyst client
# Connects to localhost:16383, logs in as test/test, enters the world
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CLIENT_BUILD="$SCRIPT_DIR/dreadmyst-client/build"
CLIENT_BIN="$CLIENT_BUILD/DreadmystClient"

# Build if needed
if [ ! -f "$CLIENT_BIN" ]; then
    echo "Client binary not found, building..."
    mkdir -p "$CLIENT_BUILD"
    cd "$CLIENT_BUILD"
    cmake ..
    cmake --build .
    cd "$SCRIPT_DIR"
fi

# Ensure asset symlink exists
if [ ! -e "$SCRIPT_DIR/dreadmyst-client/dreadmyst-scripts-db-assets" ]; then
    ln -sf ../assets "$SCRIPT_DIR/dreadmyst-client/dreadmyst-scripts-db-assets"
fi

# Ensure content symlink in build dir
if [ ! -e "$CLIENT_BUILD/content" ]; then
    ln -sf ../../assets/content "$CLIENT_BUILD/content"
fi

# Write client config with auth credentials
cat > "$CLIENT_BUILD/config.ini" << 'EOF'
[System]
DevMode=true

[Window]
Width=1280
Height=720
MaxFPS=60
Fullscreen=false
Resize=true
WindowName=Dreadmyst
OriginalDPI=false

[Tcp]
Address=127.0.0.1
Port=16383

[Auth]
Username=test
Password=test
EOF

echo "Starting Dreadmyst Client (connecting to 127.0.0.1:16383 as test/test)..."
cd "$CLIENT_BUILD"
exec ./DreadmystClient
