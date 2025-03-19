#!/bin/bash

/usr/bin/jhttp.sh -s https -r localhost -t 5 --port 1443 --host storage-idm.private-api.cloud.yandex.net --url /ping?1123123  -o -k
