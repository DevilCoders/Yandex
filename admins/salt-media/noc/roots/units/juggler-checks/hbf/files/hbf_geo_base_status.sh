#!/usr/bin/env bash
CONF="/etc/nginx/conf.d/geo.conf"
[ -n "$(find ${CONF} -mmin +172800)" ] && echo "PASSIVE-CHECK:geo_base_actual;2;$CONF is stale" || echo "PASSIVE-CHECK:geo_base_actual;0;OK"
