#!/bin/sh

cd `dirname $0`/..
YANDEX_TOKEN=`cat ~/.ssh/yav_token`

terraform apply --var yandex_token=${YANDEX_TOKEN}
