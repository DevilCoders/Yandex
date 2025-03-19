#!/bin/sh
#
#
#

me=${0##*/}
me=${me%.*}
BASE=$HOME/agents
CONF=$BASE/etc/$me.conf
LOGDEFAULT=/var/log/corba/minihyper.log

die () 
{
	
	echo "$1;$2" 
	exit
}

[ -s $CONF ] && . $CONF

FILELOG=${LOG:-$LOGDEFAULT}

[ -s $FILELOG ] || die 2 "log file not found" 

filedate=`ls -l $FILELOG | awk '{ print $7 }'`

curhour=`date '+%H'`	# Current hour
curmin=`date '+%M'`	# Current minutes
cursec=`date '+%S'`	# Current seconds
curdate=`date '+%d'`	# Current date


# Calculate time in seconds from 00:00:00
ts=`expr $curhour '*' 60 '*' 60 + $curmin '*' 60 + $cursec`

set -- `tail -n 30000 $FILELOG | grep DEBUG | grep reaver_ir | grep "Reload Complete" | awk -v t=$ts '{
	# define date
	split($3,d,":");
	
	t_delta = 300;	# 5min 
	big_delta = 300*6;

	t_limit = t - t_delta 
	big_limit = t - big_delta;
	
	if (t_limit < 0) 
	{
		t_limit = 0;
	}
	
	if (big_limit < 0) 
	{
		big_limit = 0;
	}
	
	hour = d[1];
	min = d[2]; 
	sec = d[3];	

	# Request time in the LOG in sec
	line_time_sec = 60*60*hour + 60*min + sec; 
 
	if (line_time_sec > t_limit) 
	{
		t_count = t_count + 1;
	}

	if (line_time_sec > big_limit) 
	{
		big_count = big_count + 1;
	}
} END { 
	printf "%d %d %d %d", t_count, big_count, t_limit, big_limit;
}'`

count1=$1
count2=$2
limit1=$3
limit2=$4

[ $count1 -gt 0 ] && die 0 "OK!"
[ $count2 -gt 0 ] && die 1 "$count2 lines for 30 min"
die 2 "0 lines for 30 min" 

exit


