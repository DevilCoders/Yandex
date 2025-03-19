#!/bin/bash

# check for *-error files in /var/log/mirror/

ERROR_FILES=$(find /var/log/mirrors -name '*-error' -exec basename {} \;)

if [ "x$ERROR_FILES" != "x" ]; then
    echo "1;Some errors during sync found (No route to host or module problem): $(echo $ERROR_FILES)"
else
    echo "0;ok"
fi
