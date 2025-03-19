#!/bin/bash

if [ -z "$INSTALLATION" ] && [ -f /installation ]
then
    INSTALLATION=`cat /installation`
fi

if [ -e /config ]
then
    rm -rf /config/*
fi

CONFIG_SOURCE="/config-sources/${INSTALLATION:-default}"
if [ ! -e "$CONFIG_SOURCE" ]
then
    CONFIG_SOURCE="/config-sources/default"
fi

set -e
cp -Rf /config-sources/common/* /config/
cp -Rf $CONFIG_SOURCE/* /config/

if [ -e /config/tvm/decrypt_bundle.sh ]
then
    /config/tvm/decrypt_bundle.sh
fi

touch /config/ready
echo /config was setted from "$CONFIG_SOURCE"
