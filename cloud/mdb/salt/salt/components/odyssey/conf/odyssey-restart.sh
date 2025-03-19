{% set restart_file = '/tmp/restart.signal' %}
{% set odyssey_user = salt['pillar.get']('data:odyssey:user', 'postgres') %}
#!/bin/bash

MAINPID=$1
if [ -f {{ restart_file }} ]; then
    sudo -u {{ odyssey_user }} /usr/bin/odyssey /etc/odyssey/odyssey.conf
	/bin/kill -s USR2 $MAINPID
    rm -fr {{ restart_file }}
else
    /bin/kill -s HUP $MAINPID
fi

