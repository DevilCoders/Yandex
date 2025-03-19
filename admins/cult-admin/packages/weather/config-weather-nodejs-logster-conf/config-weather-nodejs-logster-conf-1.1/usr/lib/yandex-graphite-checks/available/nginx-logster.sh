#!/bin/bash
/usr/sbin/logster -d --state-dir=/tmp/ --parser-option="/etc/logster/RPSTimingsLogster.conf" --output=stdout  RPSTimingsLogster /var/log/nginx/access.log | awk '{print "nginx." $0}'
