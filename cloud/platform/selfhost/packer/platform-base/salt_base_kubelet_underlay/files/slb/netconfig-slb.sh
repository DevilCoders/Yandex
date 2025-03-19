#!/bin/bash

set -e

CONFIG_FILE=/etc/network/interfaces.d/11_yc_slb
FQDN=`curl -s http://169.254.169.254/latest/meta-data/hostname`
if [ -e "$CONFIG_FILE" ]; then
  OLD_CONFIG=`cat $CONFIG_FILE`
else
  OLD_CONFIG=""
fi
NEW_CONFIG=`netconfig --fqdn=$FQDN`

if [ "$OLD_CONFIG" == "$NEW_CONFIG" ]; then
  echo "SLB configs are the same, exiting"
  exit 0
fi

echo "Updating $CONFIG_FILE"

ifdown -a -X lo -X eth0

ip addr flush lo:200
ip addr flush lo:201
ip addr flush lo:202
ip addr flush lo:203
# See 99_yc_metadata
ip addr flush lo:1337
ip addr flush lo:1338

echo -e "$NEW_CONFIG" > $CONFIG_FILE

ifup -a -X lo -X eth0
