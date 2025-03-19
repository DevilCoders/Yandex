#!/bin/bash

for ip in `yc compute ig list-instances ${1} | grep " 2a02:" | cut -d "|" -f 5`
  do pssh -B bastion.cloud.yandex.net ${ip} "cat /var/log/yc/ai/services-proxy* | grep -i \"${2}\" | tail"
done
