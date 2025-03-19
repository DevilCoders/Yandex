#!/bin/bash

check_prefix="PASSIVE-CHECK:salt-master-alive;"

if ! ps aux|grep [/]usr/bin/salt-master > /dev/null; then
   echo "${check_prefix}2; Master is down"
else
   echo "${check_prefix}0; Ok"
fi

