#!/bin/bash

exec /usr/bin/envoy -c $ENVOY_CONFIG_FILE $ENVOY_START_OPTS --restart-epoch $RESTART_EPOCH | tee
