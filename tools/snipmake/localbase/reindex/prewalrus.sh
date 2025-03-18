#!/usr/local/bin/bash

ARCADIA_PATH="../../../../"

if [ -f prewalrus ]
then
    ./prewalrus -R -h .
else
    echo "ERROR:  prewalrus doesn't exist"
    exit 1
fi

