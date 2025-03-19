#!/usr/bin/env bash

LIBSSL=`dpkg -l libssl* | grep ii | awk '{ print $2 }'`

apt-get update >/dev/null && apt-get install $LIBSSL >/dev/null

LIST=$( for i in `dpkg -L $LIBSSL|grep /lib/` ; do if [ -f $i ] ; then echo $i ; fi ; done )
LIST=`for i in $LIST ; do lsof -n $i|tail -n+2 ; done|awk '{print $2}'|sort|uniq`
LIST=$(for i in $LIST ; do echo "`readlink /proc/$i/exe`"; done | sort | uniq)

RESTART=$(for i in $LIST; do
    BINARY=$i
    if [ -x $BINARY ]; then
        PACKAGE=`dpkg -S $BINARY | cut -f 1 -d ':'`
        SCRIPT=`dpkg -L $PACKAGE | grep '/etc/init.d/'`
    fi
    if [ -z $SCRIPT ]; then
        SCRIPT="/etc/init.d/"`ls -1 /etc/init.d/ | grep $(basename $BINARY)`
    fi
        SERVICE=`basename $SCRIPT`
        echo "$SERVICE"
done | sort | uniq)

for s in $RESTART; do
    service $s --full-restart
done
