#!/bin/bash
LOG="/var/log/nginx/access.log"
TMPLOG="/var/tmp/timetail-nginx-log-1m.tmp"
timetail -n 60 $LOG > $TMPLOG
