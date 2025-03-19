#!/bin/sh -e
. /etc/default/redis-server

exec /usr/bin/redis-server /etc/redis/redis.conf $EXTRA_CMD_PARAMS
