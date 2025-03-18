#!/bin/sh

#TODO: write instruction

#preparing launch nginx
echo 'start preparing launch nginx'

chmod +x /etc/nginx/make_conf.sh
/etc/nginx/make_conf.sh

/usr/bin/slb ensure_open
service nginx stop
echo 'finished preparing launch nginx'
