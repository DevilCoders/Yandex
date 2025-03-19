#!/bin/bash


USER="`whoami`"
HOST="`uname -n | sed 's/[^a-zA-Z0-9]/_/g'`"

case $USER in
  "heios")
    CTYPE="${USER}_${HOST}_service_monitor"
    BASE_PORT=1400
    CONTROLLER_PORT=1401
    LOG_DIR="`pwd`/logs$BASE_PORT"
    ;;

  *)
    CTYPE="stable_platform"
    BASE_PORT=1200
    CONTROLLER_PORT=1201
    LOG_DIR="`pwd`/logs"
    ;;
esac

OPT_GDB=""
if [ "$1" == "-g" ]; then
    OPT_GDB="gdb -ex run --args";
fi

set -x

$OPT_GDB \
./service_monitor configs/config \
    -V CType=$CTYPE \
    -V LogDir=$LOG_DIR \
    -V ControllerPort=$CONTROLLER_PORT \
    -V BasePort=$BASE_PORT
