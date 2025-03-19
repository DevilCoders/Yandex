#!/bin/bash

sleep `/usr/bin/expr $RANDOM \% 1800`

/usr/sbin/logrotate -vf /etc/logrotate.conf > /var/log/force-logrotate.log 2>&1

