#!/bin/bash
set -e

TOKEN=`ya vault oauth`

cat << EOF
{"token": "$TOKEN"}
EOF
