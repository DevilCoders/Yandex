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
GRAPH_CACHE_ENABLED=false ./asr-server asr-server.xml > /var/log/asr/server.log 2>&1 &

child=$! 
wait "$child"
