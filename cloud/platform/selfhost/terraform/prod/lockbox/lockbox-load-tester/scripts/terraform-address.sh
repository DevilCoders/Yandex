#!/bin/sh

YC_TOKEN=`cat ~/.ssh/yc_token`
YAV_TOKEN=`cat ~/.ssh/yav_token`

exec terraform $* -target "ycp_vpc_inner_address.lb-address" -var yc_token=${YC_TOKEN} -var yandex_token=${YAV_TOKEN}
