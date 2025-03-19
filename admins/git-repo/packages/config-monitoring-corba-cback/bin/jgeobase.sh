#!/bin/bash

#/var/cache/geobase/geodata4.bin = 25 hours
#/var/cache/geobase/geodata-local4.bin = 3 hours

GEODATA_TIME=$((25 * 60))
GEODATA_LOCAL_TIME=$((1 * 60))

geodata-local4.bin () {
time=$(find /var/cache/geobase/ -maxdepth 1 -mmin +${GEODATA_LOCAL_TIME} -iname geodata-local4.bin -print| wc -l)
if [[ $time == 0 ]]; then 
    echo "0; OK"
else echo "2; find old /var/cache/geobase/geodata-local4.bin" 
fi
}

geodata4.bin () {
time=$(find /var/cache/geobase/ -maxdepth 1 -mmin +${GEODATA_TIME} -iname geodata4.bin -print | wc -l)
if [[ $time == 0 ]]; then
    echo "0; OK"
else echo "2; find old /var/cache/geobase/geodata4.bin"
fi
}

$1

