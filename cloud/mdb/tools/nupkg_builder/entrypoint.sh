#!/bin/bash

set -ex

cd /package

echo "COMMAND: $1"
echo "DIR"
ls -lah .
echo "ENVIRONMENT"
env

case "$1" in
    build)
        exec nuget pack
        ;;
    upload)
        shift
        exec /Sleet/Sleet push -c /etc/sleet.json $@
        ;;
    sleet)
        shift
        cmd=$1
        shift
        exec /Sleet/Sleet $cmd -c /etc/sleet.json $@
        ;;
    shell)
        exec /bin/bash
        ;;
    *)
        echo "unexpected command: $@"
        exit 1
        ;;
esac
