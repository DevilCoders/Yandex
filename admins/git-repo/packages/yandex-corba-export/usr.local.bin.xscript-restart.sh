#!/bin/bash

xscriptname=$1
memory_limit=2500000
if file /bin/bash | grep -q '64-bit'
then
	memory_limit=$[$memory_limit*5/3]
fi

if [ "x$xscriptname" = "x" ]
then
  echo Not enough parameters. Name of the xscript instance needed.
  exit 1
fi

xscript_process=`ps ax | grep "/usr/bin/xscript-bin --config=/etc/xscript-multiple/conf-enabled/$xscriptname/xscript.conf" | grep -v grep`
if [ "x$xscript_process" != "x" ]; then
        size=`ps axu | grep "/usr/bin/xscript-bin --config=/etc/xscript-multiple/conf-enabled/$xscriptname/xscript.conf" | grep -v grep | awk '{print $6}'`
        if [ "$size" -ge "$memory_limit" ]; then
                date -R >> /var/log/xscript/xscriptstart.log
                echo "$xscriptname size is $size. Restarting." >> /var/log/xscript/xscriptstart.log
		/usr/local/sbin/rexscript $xscriptname >/dev/null 2>&1
        fi
fi
exit 0

