#!/bin/bash
if test -e /etc/init.d/disable-ra; then
    msg="$(/etc/init.d/disable-ra monrun)"
else
    msg="1;config-init-disable-ra not installed?"
fi
echo "PASSIVE-CHECK:autov6here;$msg"
