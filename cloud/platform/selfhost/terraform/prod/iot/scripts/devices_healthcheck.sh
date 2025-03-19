#!/usr/bin/env bash

source "${BASH_SOURCE%/*}/common.sh"

source "${BASH_SOURCE%/*}/common.sh"

if [[ "$#" -ne "2" ]]; then
    echo "Usage: $0 LOCATION INDEX" 1>&2
    exit 1
fi

HOSTS=`get_instance_field "devices" $@ "hostname"`
exit_if_error "$?"

EXIT_CODE="0"
for HOST in $HOSTS
do
    nc -w2 -zv "$HOST" "443" &>/dev/null
    if [[ "$?" -ne "0" ]]; then
        echo "Host $HOST is DOWN"
        EXIT_CODE="1"
    else
        echo "Host $HOST is UP"
    fi
done
exit "$EXIT_CODE"
