#!/bin/bash

ifname="vlan688"

out=$(ifconfig $ifname 2>/dev/null)
if [[ $? == 0 ]]; then
  if [[ `echo $out | egrep -o "inet6 addr: [:a-f0-9/]* Scope:" | wc -l` == 3 ]]; then
    echo "0;OK"
  else
    echo "2;CRIT: Some address missed on $ifname interfaces "$(echo $out | grep "inet6 addr:" | tr "\n" " ")
  fi
else
  echo "2;CRIT: Interface $ifname missed!"
fi

