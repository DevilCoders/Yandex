#!/bin/bash

if ! ps aux|grep [/]usr/bin/salt-master > /dev/null; then
   echo "2; Master is down"
else
   echo "0; Ok"
fi
