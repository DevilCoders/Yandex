#!/bin/sh
#

setlevel() {
    if [ ${level:-0} -lt $1 ]
    then
        level=$1
    fi
}

die() {
    IFS=';'
    echo "${level:-0};${msg[*]:-OK}"
    exit 0
}

if [ ! -e /usr/sbin/hw_watcher ]; 
then
    die 0 "Not hw_watcher"
fi

HW_WATCHER='/usr/sbin/hw_watcher'
if [ $UID = 0 ]; then
    HW_WATCHER='sudo -u hw-watcher /usr/sbin/hw_watcher'
fi

for module in $@
do
    info=`$HW_WATCHER $module extended_status 2>/dev/null`
    status=`echo $info | jq '.status' | sed 's/"//g'`
    reason=`echo $info | jq '.reason' | sed 's/"//g'`
    out_reason=`echo $info | jq '.reason' | sed 's/"//g' |grep -v -e '\[' -e '\]' | sed "s/can't/cannot/g" | xargs`
    if [ "x$status" != "xOK" ] && [ "x$status" != "xWARNING" ]
    then
        msg[m++]="$module: ($out_reason)"
    fi
    case $status in
        NOTSET)
            setlevel 1;;
        OK)
            setlevel 0;;
        WARNING)
            out_reason=`echo "$reason" |grep -v -e 'Airflow_Temperature_Cel' -e 'not aligned to 4K' -e '^\[' -e '^\]' -e 'SSD wearout attribute' | sed "s/can't/cannot/g" | xargs`
            if [ ! -z "$out_reason" ]
            then
                msg[m++]="$module: ($out_reason)"
                setlevel 1
            fi
        ;;
        FAILED)
            setlevel 1;;
        RECOVERY)
            setlevel 1;;
        UNKNOWN)
            setlevel 2;;
    esac
done

if [ -e /var/tmp/hw-watcher-hook-retry ]
then
    err=`find /var/tmp/ -iname hw-watcher-hook-retry -ctime +7 2>/dev/null`
    if [ -z $err ]
    then
        msg[m++]="Hook retry return code"
        setlevel 1
    else
        msg[m++]="Hook retry return code over 7 days"
        setlevel 2
    fi
fi

n_warn=$(find /var/tmp -maxdepth 1 -name 'hw-watcher-mastermind-fail-*' | wc -l);
n_fails=$(find /var/tmp -maxdepth 1 -name 'hw-watcher-mastermind-fail-*' -mmin +60 | wc -l);

if [ $n_fails -eq 0 ]; then
	if [ $n_warn -gt 0 ]; then
		msg[m++]="$n_warn masterwrap errors found, retrying"
		setlevel 1
	fi
else
	msg[m++]="$n_fails masterwrap errors found"
	setlevel 2
fi

die
