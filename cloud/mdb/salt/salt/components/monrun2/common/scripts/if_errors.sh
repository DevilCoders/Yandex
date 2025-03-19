#!/bin/bash

me=${0##*/}     # strip path
me=${me%.*}     # strip extension
TMP="/tmp"

# get all active real interfaces
IFACE=$(/sbin/ifconfig | awk  '( $0 !~ /^ / && $1 !~ /^lo/ && $1 !~ /:/) { print $1 }')

die () {
    echo "$1;$2"
    exit 0
}

while getopts "c:" OPTION
do
    case $OPTION in
        c)
            CRIT_LIMIT="$OPTARG"
        ;;
    esac
done

crit_limit=${CRIT_LIMIT:-100}

for i in $IFACE
do
    rxerrors=$(/sbin/ifconfig "$i" | fgrep 'RX packets' | grep -o -E "errors:([0-9]*)" | cut -d ":" -f 2)
    txerrors=$(/sbin/ifconfig "$i" | fgrep 'TX packets' | grep -o -E "errors:([0-9]*)" | cut -d ":" -f 2)

    if [ -s "$TMP/$me.$i.rx" ]; then
        oldrxerrors=$(cat "$TMP/$me.$i.rx")
    else
        oldrxerrors=0
    fi

    if [ -s "$TMP/$me.$i.tx" ]; then
        oldtxerrors=$(cat "$TMP/$me.$i.tx")
    else
        oldtxerrors=0
    fi

    if [ "$oldrxerrors" -ge "$rxerrors" ]; then
        oldrxerrors=$rxerrors
    fi

    if [ "$oldtxerrors" -ge "$txerrors" ]; then
        oldtxerrors=$txerrors
    fi

    echo "$rxerrors" > "$TMP/$me.$i.rx"
    echo "$txerrors" > "$TMP/$me.$i.tx"

    deltarx=$(( rxerrors - oldrxerrors ))
    deltatx=$(( txerrors - oldtxerrors ))

    if [ "$deltarx" -ge "$crit_limit" ]; then
        die 2 "RX errors on $i: $deltarx";
    fi

    if [ "$deltatx" -ge "$crit_limit" ]; then
        die 2 "TX errors on $i: $deltatx";
    fi
done

die 0 "OK"
