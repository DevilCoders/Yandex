#!/bin/sh

. /usr/local/sbin/autodetect_environment

die() {
    echo "PASSIVE-CHECK:ipmi;$1;$2"
    exit 0
}

if [ "$is_virtual_host" -eq 1 ]; then
    die OK "virtual CT, skip ipmi checking"
fi

which hw_watcher 1>/dev/null 2>/dev/null
hw_watcher_status=$?

if [ "$hw_watcher_status" -ne 0 ]; then
    die WARN "hw_watcher not found"
fi

bmc_output=$(sudo -u hw-watcher hw_watcher bmc status)
bmc_output_status=$(echo "$bmc_output" | sed 's/WARNING;\ /1;/g' | sed 's/CRITICAL;\ /2;/g' | sed 's/OK;\ /0;/g' | sed 's/FAILED;\ /2;/g')
die "$bmc_output_status" "$bmc_output"
