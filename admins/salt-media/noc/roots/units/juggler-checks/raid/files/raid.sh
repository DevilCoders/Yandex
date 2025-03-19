#!/bin/bash
#
# $Id: raid.sh,v 1.3 2007/11/03 10:02:08 andozer Exp $
#
me=${0##*/}	# strip path
me=${me%.*}	# strip extension
BASE=$HOME/agents
CONF=/etc/monitoring/${me}.conf
TMP=/tmp
PATH=/bin:/sbin:/usr/bin:/usr/sbin
STAT_OID=/proc/mdstat
err_c=0


die () {
	echo "PASSIVE-CHECK:$me;$1;$2"
	exit 0
}

#
# check openvz CT via yandex-lib-autodetect-environment package
#
if [ -f /usr/local/sbin/autodetect_environment ] ; then
		. /usr/local/sbin/autodetect_environment
		if [ $is_virtual_host -eq 1 ] ; then
				die 0 "OK, virtual CT, skip raid checking"
		fi
fi

[ -s $CONF ] && . $CONF

[ -r $STAT_OID ] || die 2 "Can not read $STAT_OID"
# Get arrays
[ "$(cat $STAT_OID | tail -n +2 | grep raid | cut -d " " -f 1)" = "" ] && die 2 "md raid not configured" #no arrays found

set `cat $STAT_OID | tail -n +2 | grep raid | cut -d " " -f 1`
for i in "$@"
do
	i=/dev/$i
	# prepare data for analysis
	set -- `sudo /sbin/mdadm --detail $i 2>/dev/null | grep 'faulty\|remove\|degraded'`
	i=${i##*\\}

	[ "$*" ] || continue

	shift 4

	ERR="$i:$*"

	set -- `grep recovery $STAT_OID`

	[ "$*" ] && {
		ERR="$ERR $2 $3 $4" 
		set --
		set -- `echo $ERR | grep '%'`
		[ "$*" ] && {
			[ "$err_c" == "2" ] || err_c=1
			ERR_MES="$ERR_MES $ERR"
			continue
		}
	}

	case "$ERR" in
	*faulty*)	err_c=2
			;;
	*removed*)	set -- `grep -A1 $i $STAT_OID | tail -1`
			err_c=2
			;;
	*)		err_c=2
			;;
	esac
	ERR_MES="$ERR_MES $ERR $*"

done

if cat /proc/mdstat | grep -q 'inactive'; then
	ERR_MES="${ERR_MES} inactive raid"
	err_c=2
fi

if cat /proc/mdstat | grep -q 'resync'; then
	ERR_MES="${ERR_MES} resyncing raid"
	err_c=1
fi

[ "$ERR_MES" ] || ERR_MES=OK
die $err_c "$ERR_MES"
