#!/bin/bash

ifname="vlan688"

out=$(ip -6 a l $ifname 2>/dev/null)
if [[ $? == 0 ]]; then
  if [[ $(echo "$out" | grep -c 'inet6') -eq 3 ]]; then
    echo "0;OK"
  else
    echo "2;CRIT: Some address missed on $ifname interfaces "$(echo "$out" | grep "inet6" | awk '{print $2}' | tr "\n" " ")
  fi
else
  echo "2;CRIT: Interface $ifname missed!"
fi

