#!/bin/bash

/sbin/ifconfig $1 up
/sbin/ip -f inet6 addr add fd01:ffff:ffff:ffff::1/96 dev "${1}"
