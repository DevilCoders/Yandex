#!/bin/bash

VNCSERVERS=""

[ -f /etc/default/vncservers ] && . /etc/default/vncservers || exit 0

start() {

    for display in ${VNCSERVERS}; do
        eval cd ~${display##*:}
        if [ -e ".vnc/passwd" ]; then
            su ${display##*:} -c "cd ~${display##*:} && vncserver -localhost no :${display%%:*} &> /dev/null"
        fi
    done
}

stop() {
    VNCSERVERS=$(ps aux | grep vnc | egrep -o ott.* | awk '{ print $1" "$2 }' |sed -e 's/(//g' -e 's/)//g' | awk -F ":" '{ print $2 }' | awk '{ print $1":"$2}' | tr '\n' ' ')

    for display in ${VNCSERVERS}; do
        su ${display##*:} -c "vncserver -kill :${display%%:*}" >/dev/null 2>&1
    done
}

case "$1" in
    start)              start
                        ;;

    stop)               stop
                        ;;

    restart|reload)     stop
                        start
                        ;;

    *)                  echo $"Usage: $0 {start|stop|restart}"
                        exit 1
                        ;;

esac

exit 0