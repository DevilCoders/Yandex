#!/bin/bash
cd /mirror/mirrors
wget -nc -m -e robots=off -np --convert-links -A="*.msi*, *-linux-x64.tar.gz*, *-linux-x86.tar.gz*, *-x64.msi*, *-x86.msi*, *.pkg*, *-linux-arm64.tar.gz*, *-linux-armv7l.tar.gz, *-darwin-x64.tar.gz*, *SHASUMS256.*, *npm-versions.txt*, *index.json*, *index.tab*" -X "*/v0*" http://nodejs.org/dist/
