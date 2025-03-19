#!/bin/bash

SLEEP_INTERVAL=300

_term() {
  echo "Preparing for termination, waiting for $SLEEP_INTERVAL seconds"
  sleep $SLEEP_INTERVAL
  kill "$child"
  wait "$child"
}

trap _term SIGTERM

echo "Starting wrapper script";
./asr_server --config-path /model/lingware/asr_system_config.json --port 17004 --unistat-port 17002 > /var/log/asr/server.log 2>&1 & 

child=$!
wait "$child"
