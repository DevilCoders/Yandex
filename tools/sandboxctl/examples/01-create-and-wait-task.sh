#!/bin/bash

bin/sandboxctl create  -N RUN_SCRIPT \
       -c cmdline='dmesg > dmesg.txt; lscpu > lscpu.txt' \
       -C '{"save_as_resource" : {"dmesg.txt":"OTHER_RESOURCE", "lscpu.txt": "OTHER_RESOURCE"}}' \
       -W
