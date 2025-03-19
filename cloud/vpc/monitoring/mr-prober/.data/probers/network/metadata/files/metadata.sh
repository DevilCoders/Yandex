#!/bin/bash

success() {
    echo "[ SUCCESS ] $(date '+%F %X.%N') $1"
}

fail() {
    echo "[  FAIL   ] $(date '+%F %X.%N') $1"
}

echo "[ STARTED ] $(date '+%F %X.%N')"

URL="http://169.254.169.254/latest/meta-data/$1"
RESPONSE=$(curl --fail --silent --show-error "$URL" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ]
then
    fail "Non-zero exit code '$EXIT_CODE' with '$RESPONSE' on '$URL'"
    exit 1
fi

if [ "$RESPONSE" == "$2" ]
then
    success "Metadata returned '$RESPONSE' on '$URL'"
    exit 0
else
    fail "Unexpected response '$RESPONSE' on '$URL', expected '$2'"
    exit 1
fi
