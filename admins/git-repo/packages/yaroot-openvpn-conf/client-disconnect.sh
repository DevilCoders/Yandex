#!/usr/bin/env bash

#DB="/tmp/openvpn-connections"
#sed -i "/$common_name/d" $DB

curl -m5 -d "ip=$ifconfig_pool_remote_ip&team=$X509_0_OU&user=$common_name" http://127.0.0.1/dissociate
