#!/usr/bin/env bash

#DB="/tmp/openvpn-connections"
#echo "$common_name $ifconfig_pool_remote_ip" >> $DB
curl -m5 -d "ip=$ifconfig_pool_remote_ip&team=$X509_0_OU&user=$common_name&server=$(hostname -f)" http://127.0.0.1/associate
