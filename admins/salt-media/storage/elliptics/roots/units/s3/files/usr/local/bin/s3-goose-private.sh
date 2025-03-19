#!/bin/bash

DOMAIN=$(hostname -f | egrep -o ".mds[t]?.yandex.net")
/usr/bin/jhttp.sh --server localhost -t 5 --port 3399 --host s3-idm.$DOMAIN --url /ping

