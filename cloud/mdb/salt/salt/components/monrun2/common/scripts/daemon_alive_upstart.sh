#!/bin/sh
USE_SUDO=0

die () {
    printf "%s;%s\n" "$1" "$2"
    exit 0
}


while getopts "sd:" OPTION
do
    case $OPTION in
        s)
            USE_SUDO="1"
        ;;
        d)
            DAEMON="$OPTARG"
        ;;
    esac
done

if (status "${DAEMON}" 2>&1 | fgrep -q 'start/running')
then
    die 0 "OK"
else
    die 2 "${DAEMON} not running"
fi
