#!/bin/bash

/usr/sbin/logrotate -d /etc/logrotate.conf 2> /tmp/logrotate.err;
ERR=`grep 'could not read state file' /tmp/logrotate.err | wc -l`;

/usr/sbin/logrotate -d /etc/logrotate.conf 2>/dev/null;
if [ $? -eq 0 ]; then
        echo "0;OK";
else
        if [ $ERR -eq 0 ]; then
                echo "2;Logrotate broken";
        else
                rm -f /var/lib/logrotate/status;
                /usr/sbin/logrotate -d /etc/logrotate.conf 2>/dev/null;
                if [ $? -eq 0 ]; then
                        echo "0;OK";
                else
                        echo "2;Logrotate broken";
                fi
        fi
fi
