#!/usr/local/bin/bash

# Путь к корню нужного бранча аркадии
ARCADIA_PATH="../../../.."
# Папка, в которой будут сохранены нужные бинарники
BIN_DIR="bin"

if [ ! -d $BIN_DIR ]
then
    mkdir $BIN_DIR
fi

../common/build_and_copy.sh $ARCADIA_PATH/yweb/robot/datawork datawork $BIN_DIR
../common/build_and_copy.sh $ARCADIA_PATH/yweb/robot/m2nsort m2nsort $BIN_DIR

