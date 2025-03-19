#!/bin/bash

if [ $(ubic status yandex-zookeeper-nocdev | grep running | wc -l) -ne 1 ]; then echo echo "PASSIVE-CHECK:zookeeper-status;CRIT;zookeeper not running"; else echo "PASSIVE-CHECK:zookeeper-status;OK;OK"; fi
