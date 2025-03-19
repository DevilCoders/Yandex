#!/bin/bash

mkdir -p /var/tmp/MDSTODO-28

tmp_499=`mktemp`
grep service=alice /var/log/mds/mds-access.log | grep status=499 |grep -P "request_time=[1-9]\.\d+" > $tmp_499
for trace in `cat $tmp_499 |grep -P "(?<=trace_id=)\w+" -o`
do
	# trace=`echo $l | grep -P "(?<=trace_id=)\w+" -o`
	l=`grep $trace $tmp_499`
	date=`echo $l | grep -P "(?<=timestamp=)[\w-:\.]+" -o`
	timestamp=`date --date="$date" +%s`
	thevoid_log='pidor'
	for log in `ls -1tr /var/log/mds/thevoid.log*`
	do
		modify_ts=`stat $log |grep Modify | awk '{print $2, $3}'`
		modify_timestamp=`date --date="$modify_ts" +%s`
		if [ $modify_timestamp -gt $timestamp ]
		then
			thevoid_log=$log
			break
		fi
	done

	tmp_thevoid=`mktemp`
	zstdgrep $trace $thevoid_log > $tmp_thevoid
	cat $tmp_thevoid |grep -q "Operation timed out after 5"
	if [ $? -eq 0 ]
	then
		connect_time='0.000'
		total_time='5.000'
	else
		connect_time=`cat $tmp_thevoid |grep "received HTTP response" |grep -P "(?<=connect_time=)[\d\.]+" -o`
		total_time=`cat $tmp_thevoid |grep "received HTTP response" |grep -P "(?<=total_time=)[\d\.]+" -o`
	fi
	ip=`cat $tmp_thevoid |grep "initialized HTTP checker" |grep -P "2a02:06b8[:\w]+" -o | head -n1`
	hostname=`host $ip | awk '{print $NF}' | sed 's/.$//g'`
	rm $tmp_thevoid

	tmp_storage=`mktemp`
	ssh root@$hostname "grep $trace /var/log/nginx/tskv.log /var/log/elliptics/srw/spacemimic/access.log" > $tmp_storage
	mimic_time=`cat $tmp_storage | grep 'spacemimic/access.log' | awk '{print $NF}'`
	nginx_upstream_time=`cat $tmp_storage | grep 'nginx/tskv.log' | grep -P "(?<=upstream_response_time=)[\d\.]+" -o`
	nginx_processing_time=`cat $tmp_storage | grep 'nginx/tskv.log' | grep -P "(?<=processing_time=)[\d\.]+" -o`
	rm $tmp_storage

	echo -e "trace_id=$trace\tdate=$date\ttimestamp=$timestamp\ttotal_time=$total_time\tconnect_time=$connect_time\thostname=$hostname\tmimic_time=$mimic_time\tnginx_upstream_time=$nginx_upstream_time\tnginx_processing_time=$nginx_processing_time\tproxy_host=`hostname -f`"
done

rm $tmp_499
