#! /usr/local/bin/bash

ARC_PATH=$1
BIN_NAME=$2
BIN_DIR=$3

echo -n "$BIN_NAME: "
if [ ! -d $ARC_PATH ]
then
    echo "Direcory unexist: $ARC_PATH"
    exit 1
fi

echo -n "building..."
om $ARC_PATH > /dev/null 2> /dev/null < /dev/null

echo -n "copying..."

if [ -f $ARC_PATH/$BIN_NAME ]
then
    echo "OK"
    cp -R -P -f "$ARC_PATH/$BIN_NAME" "$BIN_DIR/$BIN_NAME"
else
    echo "ERROR"
    exit 1
fi

