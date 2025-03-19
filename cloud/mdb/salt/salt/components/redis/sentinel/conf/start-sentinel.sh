#!/bin/sh -e
. /etc/default/redis-sentinel

exec /usr/bin/redis-sentinel /etc/redis/sentinel.conf $EXTRA_CMD_PARAMS
