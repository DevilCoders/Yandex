#!/bin/bash
HOST=`hostname -f`
TYPES="A AAAA"
PORT="2181"
MODE=`echo stat | nc.openbsd -w 1 -q 1 "$HOST" "$PORT" | grep "Mode" | awk '{print $NF}'`

for t in $TYPES; do
	IP=`host -t $t $HOST | awk '{print $NF}'`
	if [[ "$IP" != "record" ]]; then
		check=`echo stat | nc.openbsd -w 1 -q 1 "$IP" "$PORT" >/dev/null 2>&1`
		if [[ "$?" -gt "0" ]]; then
			ans=`echo "$ans;$t failed!"`
		fi
	fi
done

if [[ -z $ans ]]; then
	if [[ -z $MODE ]]; then
		echo "2;Zookeeper not connected. Need >=3 machines for work"
	else
		echo "0;OK, $MODE"
	fi
else
	echo "2;Zookeeper down at ipv4 or ipv6$ans"
fi
