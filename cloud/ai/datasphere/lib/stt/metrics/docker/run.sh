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
./run_service normalizer > /var/log/stt_metrics/normalizer.log 2>&1 &

child=$!
wait "$child"
