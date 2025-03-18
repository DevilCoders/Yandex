#!/usr/local/bin/bash

rsh ordure "zcat /place/logs/reqans/2010/01/reqans_log.sfront4-*.20100108.gz" | awk -F@@ '{print $1}' | sed 's/http:\/\///g'
#rsh ordure "zcat /place/logs/reqans/2010/01/reqans_log.sfront4-*.20100116.gz" | awk -F@@ '{print $1}' | sed 's/http:\/\///g'
