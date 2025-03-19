#!/bin/bash
/etc/init.d/syslog-ng restart
/etc/init.d/django restart all
/etc/init.d/gunicorn restart all
/etc/init.d/nginx restart

