#!/bin/bash
BASE=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
LOCK=/var/lock/ib-mon-ufm-state-check.lock
VAR=$BASE/var
LOG_DIR=/var/log/ib-mon
DEBUGLOG=$LOG_DIR/ufm-state-check.log

#[ "$1" == "--manual" ] || exit 0

[ -f $LOCK ] || touch $LOCK
exec 200<$LOCK
if ! flock --nonblock 200; then
	echo "`date +%Y-%m-%dT%H-%M-%S` cannot obtain lock" >> $DEBUGLOG
	exit 0
fi

date +%Y-%m-%dT%H-%M-%S >> $DEBUGLOG

check_process()
{
	local process=$1
	local service=$2
	local desc=$3

	if pgrep -f $process >/dev/null; then
		st="OK"
	else
		st="CRIT"
	fi
	echo "PASSIVE-CHECK:$service;$st;$desc"
}

(
# UFM in docker-container
[ -f $VAR/no-ufm ] || if [ -n "$(which docker 2>/dev/null)" ]; then
	if sudo /usr/bin/docker ps --filter=name=ufm --format='{{.Status}}' | grep -q Up; then
		echo "PASSIVE-CHECK:docker-ufm;OK;UFM-container status"

		check_process /usr/libexec/mysqld mysql-status "MySQL process"
		check_process /usr/sbin/httpd httpd-status "HTTPD process"
		check_process /opt/ufm/gvvm/model/ModelMain.pyc ufm-modelmain-status "ModelMain  process"
		check_process /opt/ufm/ufmhealth/UfmHealthRunner.pyc ufm-healthrunner-status "UfmHealthRunner process"
	else
		echo "PASSIVE-CHECK:docker-ufm;CRIT;UFM-container not running"
	fi | tee -a $DEBUGLOG
fi

# UFM at bare-metal centos
if [ -x /etc/init.d/ufmha ]; then
	sudo /etc/init.d/ufmha status | tee -a $DEBUGLOG | awk '
		BEGIN {
			mysql_status="CRIT";
			ufm_status="CRIT";
			drbd_state="CRIT";
			drbd_device_state="CRIT";
		}
		/Remote Host/ { exit }
		/Mysql .*Running/ { mysql_status="OK"; }
		/UFM Server .*Running/ { ufm_status="OK"; }
		/DRBD State .*(Primary|Secondary)/ { drbd_state="OK"; }
		/DRBD Device State .*UpToDate/ { drbd_device_state="OK"; }

		END {
			printf "PASSIVE-CHECK:mysql-status;%s;MySQL process\n", mysql_status;
			printf "PASSIVE-CHECK:ufm-status;%s;ufmha service\n", ufm_status;
			printf "PASSIVE-CHECK:drbd-state;%s;\n", drbd_state;
			printf "PASSIVE-CHECK:drbd-device-state;%s;\n", drbd_device_state;
		}
		'
		check_process /usr/sbin/httpd httpd-status "HTTPD process"
		check_process /opt/ufm/gvvm/model/ModelMain.pyc ufm-modelmain-status "ModelMain  process"
		check_process /opt/ufm/ufmhealth/UfmHealthRunner.pyc ufm-healthrunner-status "UfmHealthRunner process"
fi | tee -a $DEBUGLOG

# opensm should be every where
# use part of path because in ufm opensm lives in /opt/ufm/opensm/sbin/opensm
check_process /sbin/opensm opensm-running "opensm process" | tee -a $DEBUGLOG

if sudo /usr/sbin/sminfo | tee -a $DEBUGLOG | grep -q SMINFO_MASTER; then
	echo "PASSIVE-CHECK:opensm-reply;OK;OpenSM responding status"
else
	echo "PASSIVE-CHECK:opensm-reply;CRIT;OpenSM responding status"
fi | tee -a $DEBUGLOG

if /opt/ib-mon/is-master.sh; then
	echo "PASSIVE-CHECK:opensm-master;OK;OpenSM master status"
else
	echo "PASSIVE-CHECK:opensm-master;CRIT;OpenSM master status"
fi | tee -a $DEBUGLOG

) | ( $BASE/send2juggler.py -d --log-file $LOG_DIR/ufm-state-check-send2juggler.log || : )

