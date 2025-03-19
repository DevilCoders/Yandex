#!/bin/bash

function die {
    echo "$1; $2"
    exit $1
}

intel_ifaces="$(/usr/local/bin/bootutil64e)"

grep -q '10GbE' <<< "$intel_ifaces" || die 0 "No intel 10G NICs here"
grep '10GbE' <<< "$intel_ifaces" | grep -q -i 'flash disabled' && die 2 "10G adapter is not flashed"
die 0 "10G adapter is flashed"
