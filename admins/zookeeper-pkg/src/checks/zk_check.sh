#!/bin/sh -e

SERVER=`hostname --long`
CLIENT_PORT=`cat /etc/yandex/@dir@/zoo.cfg | grep "clientPort" | awk -F "=" '{print $2}'`
SERVER_PORT=`cat /etc/yandex/@dir@/zoo.cfg | grep "server." | grep "$SERVER" | awk -F ":" '{print $3}'`

if [ -f /bin/nc.openbsd ]; then
  PROGRAM="/bin/nc.openbsd"
else
  if [ -f /bin/nc ]; then
    PROGRAM="/bin/nc"
  else
    echo "Please, install netcat."
    exit 1
  fi
fi

if echo "$1" | nc -i1 localhost @port@ 2>/dev/null | egrep -q "$2"; then
  if [ `$PROGRAM -z $SERVER $SERVER_PORT; echo $?` == 0 ]; then
    exit 0;
  else
    exit 1;
  fi
else 
  exit 1; 
fi

# vim: set ts=4 sw=4 et:
