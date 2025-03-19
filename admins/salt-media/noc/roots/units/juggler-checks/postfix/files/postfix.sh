#!/bin/bash
echo "PASSIVE-CHECK:postfix;"$(/usr/sbin/daemon_check.sh postfix)
