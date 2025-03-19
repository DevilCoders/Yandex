#!/bin/bash

RESULT=$(/usr/local/teamcity-agents/`hostname -f`/bin/agent.sh status | \
              grep -c 'Service is running')

if [[ $RESULT -gt 0 ]]; then
  echo "0; Teamcity agent is alive"
else
  echo "2; Teamcity agent is dead"
fi
