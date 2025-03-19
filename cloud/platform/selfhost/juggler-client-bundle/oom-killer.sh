#!/bin/bash

CHECK_NAME="oom-killer"

LOG_PREV=/tmp/$CHECK_NAME.prev
LOG_CUR=/tmp/$CHECK_NAME.cur
LOG_DIFF_PREV=/tmp/$CHECK_NAME.diff.prev
LOG_DIFF_CUR=/tmp/$CHECK_NAME.diff.cur
TSTAMP=$(date +%s)
UPTIME=$(cat /proc/uptime | cut -f 1 -d .)
WANTED_TIME=$(( $UPTIME - 86400 ))
WATCH_LAST=3600

mexit ()
{
	error_status=$1
	error_msg=$2
	echo "PASSIVE-CHECK:$CHECK_NAME;$error_status;$error_msg"
	exit 0
}

RULES="/home/monitor/agents/etc/$CHECK_NAME.exclude"
if ! [ -e "$RULES" ]
then
	RULES="/dev/null"
fi

dmesg | grep -i -A1 "out of memory" | egrep -v -f "$RULES" | tr -d '][' | awk -v p=$WANTED_TIME '$1>=p {print}' > $LOG_CUR

if [ ! -e $LOG_PREV ]; then
	touch $LOG_PREV
fi

diff $LOG_PREV $LOG_CUR | awk -v t=$TSTAMP '{print t, $0}' >> $LOG_DIFF_PREV
awk -v p=$(( $TSTAMP - $WATCH_LAST )) '$1>=p {print}' $LOG_DIFF_PREV > $LOG_DIFF_CUR
cp $LOG_DIFF_CUR $LOG_DIFF_PREV
cp $LOG_CUR $LOG_PREV

if [ "$(cat $LOG_DIFF_PREV)" == "" ]; then
	mexit 0 "Ok"
else
	MSG_TIME=$(tail -n 1 $LOG_DIFF_PREV | sed -r 's/^[0-9]+ [><] ([0-9]+)\..*$/\1/')
	MSG_TIME=$(date --date="@$(( $TSTAMP - $UPTIME + $MSG_TIME ))" +"%d-%m-%Y %X")
	MSG=$(tail -n 1 $LOG_DIFF_PREV | sed -r "s/[0-9]* //;s/[><] //;s/^.*?Killed process ([0-9]+) \((.*?)\).*$/Process \1 (\2) killed by OOM killer in cgroup at $MSG_TIME/")
	mexit 2 "$MSG"
fi
