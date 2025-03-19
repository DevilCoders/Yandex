#!/bin/bash

die()
{
  echo "$*"
  exit 1
}

help()
{
  echo "Usage: $0 is-active SERVICE-NAME"
  exit 2
}
  
case "$1" in
  is-active) cmd=$1;;
  "") help ;;
  *) die "unsupported command '$1'"
esac

[ -n "$2" ] || help 

STATUS="$(systemctl $cmd $2)"

if [ "$STATUS" = "active" ]; then
    echo "PASSIVE-CHECK:$2;OK;OK"
else
    echo "PASSIVE-CHECK:$2;CRIT;$3"
fi

