#!/bin/sh

/sbin/ip -6 route replace default via fe80::1 dev eth0 proto ra metric 100 mtu 8950 pref medium
