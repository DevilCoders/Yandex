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
./asr_server --config-path /lingware/asr_system_config.json --port 17004 --unistat-port 17002 &

child=$!
wait "$child"
