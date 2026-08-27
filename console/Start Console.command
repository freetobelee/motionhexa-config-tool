#!/bin/bash
# Double-click this file in Finder to start the Motionhexa Console and open it
# in your browser. Leave this Terminal window open while you use it -- closing
# it (or Ctrl+C) stops the server.

cd "$(dirname "$0")"

if ! command -v node >/dev/null 2>&1; then
  echo "Node.js isn't installed."
  echo "Install it from https://nodejs.org (the LTS version), then double-click this file again."
  read -p "Press Enter to close this window..."
  exit 1
fi

PORT="${PORT:-2710}"

( sleep 1.5 && open "http://localhost:$PORT" ) &

echo "Starting Motionhexa Console at http://localhost:$PORT ..."
echo "Leave this window open. Close it (or Ctrl+C) to stop the server."
echo ""
node server.js
