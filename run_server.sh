#!/usr/bin/env bash
# Run the Dreadmyst server
# Prerequisites: MariaDB/MySQL running with 'dreadmyst' database
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SERVER_BUILD="$SCRIPT_DIR/dreadmyst-server/build"
SERVER_BIN="$SERVER_BUILD/dreadmyst-server"

# Build if needed
if [ ! -f "$SERVER_BIN" ]; then
    echo "Server binary not found, building..."
    mkdir -p "$SERVER_BUILD"
    cd "$SERVER_BUILD"
    cmake ..
    cmake --build .
    cd "$SCRIPT_DIR"
fi

# Ensure game.db symlink exists
if [ ! -e "$SERVER_BUILD/game.db" ]; then
    ln -sf ../../assets/game.db "$SERVER_BUILD/game.db"
fi

# Ensure config.ini is in build dir
if [ ! -f "$SERVER_BUILD/config.ini" ]; then
    cp "$SCRIPT_DIR/dreadmyst-server/config.ini" "$SERVER_BUILD/config.ini"
fi

# Ensure MariaDB/MySQL is running
if ! systemctl is-active --quiet mariadb 2>/dev/null && ! systemctl is-active --quiet mysql 2>/dev/null; then
    echo "ERROR: MariaDB/MySQL is not running. Start it with: sudo systemctl start mariadb"
    exit 1
fi

# Ensure database and test account exist
mariadb -u root -pdreadmyst dreadmyst -e "SELECT 1" >/dev/null 2>&1 || {
    echo "Setting up database..."
    mariadb -u root -pdreadmyst -e "CREATE DATABASE IF NOT EXISTS dreadmyst"
    mariadb -u root -pdreadmyst dreadmyst < "$SCRIPT_DIR/dreadmyst-server/sql/schema.sql"
    mariadb -u root -pdreadmyst dreadmyst -e \
        "INSERT IGNORE INTO accounts (username, password) VALUES ('test', 'test')"
    echo "Database initialized with test account (test/test)"
}

echo "Starting Dreadmyst Server on port 16383..."
cd "$SERVER_BUILD"
exec ./dreadmyst-server
