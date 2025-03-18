#!/bin/sh

echo ">>> Starting NGINX Server"
echo "========================================================================="

chmod +x /etc/nginx/make_conf.sh
/etc/nginx/make_conf.sh

slb ensure_open

nginx -g 'daemon off;'
