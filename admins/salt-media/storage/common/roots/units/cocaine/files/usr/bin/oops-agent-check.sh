#!/bin/bash

if ! [ -f /usr/bin/jq ]; then
  apt-get install -y jq
fi

STATUS=$(skyctl check -f json infra oops-agent | jq -r '.["infra"]["oops-agent"]["state"]')

if [[ $STATUS != "RUNNING" ]]; then
  RUNTIME=$(skyctl check -f json infra oops-agent | jq -r '.["infra"]["oops-agent"]["state_uptime"]')
  if [[ $RUNTIME =~ ^[0-9]*[2-9]d ]]; then
    skyctl restart infra oops-agent
    echo "0; Warn, oops-agent restart"
  elif [[ $RUNTIME =~ ^[0-9][hm] ]]; then
    echo "1; Warn, oops-agent restarted $RUNTIME ago"
  else
    echo "2; Crit, oops-agent look like not started long time, last run $RUNTIME"
  fi
else
  echo "0; OK"
fi

