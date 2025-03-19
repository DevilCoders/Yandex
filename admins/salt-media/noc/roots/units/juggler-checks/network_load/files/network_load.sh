#!/bin/bash
CHECKRES=$(/usr/bin/network_load.sh)

if [[ "$(hostname)" == "man-srv14.net.yandex.net" && $CHECKRES == "2;ACHTUNG! eth0 is operating at 1000Mb/s!" ]]; then
    CHECKRES="0;OK"
fi
echo "PASSIVE-CHECK:network_load;$CHECKRES"
