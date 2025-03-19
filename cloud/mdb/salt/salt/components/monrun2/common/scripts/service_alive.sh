#!/bin/bash

set -e

while getopts "n:" OPTION
do
    case $OPTION in
        n)
            SERVICE_NAME="$OPTARG"
        ;;
    esac
done

service_name=${SERVICE_NAME:-sshd}

die() {
    echo "$1;$2"
    exit 0
}

if service "$service_name" status >/dev/null 2>&1
then
        die 0 "OK"
else
        die 1 "DEAD"
fi
