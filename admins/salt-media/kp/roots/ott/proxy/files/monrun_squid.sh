#!/bin/sh

address="localhost"
port=3128
url="mgr:info"

res=`squidclient cache_object://$host/ $url 2>&1`
ret=$?

if [ $ret -ne 0 ] ; then 
  echo "2; `echo $res | head -1`"
else
  echo "0; alive"
fi
