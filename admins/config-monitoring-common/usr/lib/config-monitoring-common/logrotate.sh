#!/bin/bash

sudo /usr/sbin/logrotate -d /etc/logrotate.conf 2>/tmp/logrotate.log

if [ $? -eq 0 ]
        then echo "0;OK"
        else
        errors=$(grep ^error /tmp/logrotate.log | xargs)
        echo "2;Logrotate broken: ${errors}"
fi
