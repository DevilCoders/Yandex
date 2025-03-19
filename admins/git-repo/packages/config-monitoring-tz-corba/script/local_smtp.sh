#!/bin/sh
#
# Check status for SMTP@localhost
#
# $Id: local_smtp.sh,v 1.6 2007/06/29 12:47:49 andozer Exp $
#

##Script has been copied from /home/monitor/modules/ WITH SOME CHANGES!!! i removes pgrep and 'passive check'

# Environment
me=${0##*/}     # strip path
me=${me%.*}     # strip extension
CONF=$HOME/agents/etc/$me.conf
tstamp=`date '+%s'`
MAILQ_CRIT=${MAILQ_CRIT:-100}
MAILQ_WARN=${MAILQ_WARN:-50}
CHECK_PS=yes
CHECK_SMTP=no
CHECK_MAILQ=line
MAILER=exim
HOSTNAME=`hostname -f`

[ -s "$CONF" ] && . "$CONF"

die () {
	echo "$1;$2"
	exit 0
}

case "$CHECK_PS" in
[Yy][Ee][Ss])
	if [ "$MAILER" ]
	then
		[ `ps aux | grep "$MAILER" | grep -v grep | awk '{print $2}'` ] || die 2 "$MAILER not running"
	else
		[ `ps aux | grep '[a-z]mail' | grep -v grep | awk '{print $2}'` ] || die 2 "not running"
	fi
	;;
esac

case "$CHECK_SMTP" in
[Yy][Ee][Ss])
	query=${query:-quit}
	result=`echo $query | nc -w 10 ${HOSTNAME:-localhost} ${PORT:-25} 2>&1`

	test $? -ne 0 && {

		case "$result" in
			*timed*)	die 1 "$result" ;;
			*)		die 2 "$result" ;;
		esac
	}
	;;
esac

case "$CHECK_MAILQ" in
[Aa][Dd][Dd][Rr])
	Q=`mailq | grep -c '[<>]'`
	;;

[Nn][Oo])
	die 0 Ok
	;;

*)
	Q=`mailq | wc -l`
	;;
esac

if [ "$Q" -ge $MAILQ_CRIT ]
then
	die 2 "mailq: $Q entries"
elif [ "$Q" -ge $MAILQ_WARN ]
then
	die 1 "mailq: $Q entries"
fi

die 0 Ok

