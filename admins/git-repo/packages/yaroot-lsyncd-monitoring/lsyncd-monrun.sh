#! /bin/bash

pid=`ps -ef | egrep '/usr/bin/lsyncd' | egrep -v egrep | awk '{ print $2 }'`
kill -0 $pid 2>/dev/null 1>&1 && echo "0; ok" || echo "2; lsyncd not running"
