#!/bin/bash

if [[ -z `ps wuax | grep xinetd | grep -v grep` ]]
then
  /etc/init.d/xinetd start
fi
