#!/bin/bash

echo $(ip -6 addr | grep inet6 | awk -F '[ \t]+|/' '{print $3}' | grep -v ^::1 | grep -v ^fe80) $(hostname) >> /etc/hosts
