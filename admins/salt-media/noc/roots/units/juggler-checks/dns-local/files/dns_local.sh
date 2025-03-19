#!/bin/bash

#check if bind is alive and ok

res="PASSIVE-CHECK:dns_local;"

exec 2> /dev/null;

/usr/bin/host localhost localhost > /dev/null && echo "${res}0;OK" || echo "${res}2;DNS is down!"

