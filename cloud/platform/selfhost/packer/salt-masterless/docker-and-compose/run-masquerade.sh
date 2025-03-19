#!/bin/sh

/sbin/ip6tables -t nat -A POSTROUTING -s fd00::/8 -j MASQUERADE

echo "MASQUERADE is active"
