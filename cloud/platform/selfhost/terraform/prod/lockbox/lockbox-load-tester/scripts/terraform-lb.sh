#!/bin/sh

YC_TOKEN=`cat ~/.ssh/yc_token`
YAV_TOKEN=`cat ~/.ssh/yav_token`

exec terraform $* -target "ycp_load_balancer_network_load_balancer.load-balancer" -target "ycp_load_balancer_target_group.load-balancer-tg" -var yc_token=${YC_TOKEN} -var yandex_token=${YAV_TOKEN}
