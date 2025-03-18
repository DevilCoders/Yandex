#!/usr/local/bin/bash

ARCADIA_PATH=$1
BIN_DIR=$2

echo "collecting binaries..."
if [ ! -d $ARCADIA_PATH ]
then
    echo "bad arc path: $ARCADIA_PATH"
else
    ../common/build_and_copy.sh $ARCADIA_PATH/yweb/webutil/arc2dir arc2dir $BIN_DIR
    ../common/build_and_copy.sh $ARCADIA_PATH/yweb/robot/mergearc mergearchive $BIN_DIR
    ../common/build_and_copy.sh $ARCADIA_PATH/yweb/robot/prewalrus prewalrus $BIN_DIR
    ../common/build_and_copy.sh $ARCADIA_PATH/tools/tarcview tarcview $BIN_DIR
fi

