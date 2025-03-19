#!/bin/bash

LANG=ru_RU.UTF-8 exec /usr/bin/supervisord -n -c /etc/supervisor/supervisord.conf
