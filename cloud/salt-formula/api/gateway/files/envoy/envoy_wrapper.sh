#!/bin/bash

ulimit -n 102400
exec /usr/bin/envoy -c /etc/envoy/envoy.yaml \
  --log-path /var/log/envoy/error.log \
  --restart-epoch $RESTART_EPOCH
