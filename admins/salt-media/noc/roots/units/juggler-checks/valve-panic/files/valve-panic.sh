#!/bin/bash
#NOCDEV-4357

sudo journalctl --since '1 minutes ago' -u valve | grep -q 'segmentation\|panic' && echo "PASSIVE-CHECK:valve-panic;CRIT;panic detected in journalctl" || echo "PASSIVE-CHECK:valve-panic;OK;OK"
