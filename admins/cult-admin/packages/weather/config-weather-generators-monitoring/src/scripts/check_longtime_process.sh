#!/bin/bash
st=0
m=""
cts=$(date '+%s')
pid=$(ps aux | grep python | grep manage.py | grep -vE 'corba-zk-flock|flock' | awk '{print $2}' | xargs)

for i in $pid; do
	sts=$(stat -c  '%Z' /proc/$i)
	r=$((cts-sts))
	if [ "$r" -gt "14400" ]; then
		if [ "$st" -lt "2" ]; then st=2; fi
		m=${m}" Prosess $i is alive $r seconds "
	elif [ "$r" -gt "7200" ] ; then
		if [ "$st"  -lt "1" ]; then st=1 ; fi
		m=${m}" Prosess $i is alive $r seconds "
	fi
		
done

if [ $st -eq "0" ] ; then echo "0; OK"
elif [ $st -eq "1" ] ; then echo "1; $m"
elif [ $st -eq "2" ] ; then echo "2; $m"
fi

