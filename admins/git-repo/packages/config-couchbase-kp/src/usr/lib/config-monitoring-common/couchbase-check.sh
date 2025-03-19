#!/bin/bash

echo stats | nc -w 2 127.0.0.1 11211 > /dev/null 2>&1

if [ $? -ne 1 ]; then
        echo "0;OK";
        touch /usr/share/nginx/www/ping
else
        echo "2;DEAD";
        rm -f /usr/share/nginx/www/ping
fi
