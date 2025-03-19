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
    echo "PASSIVE-CHECK:hw-watcher;${level:-0};${msg[*]:-OK}"
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
    info=`$HW_WATCHER $module status 2>/dev/null`
    status=`echo $info | awk -F ';' '{print $1}'`
    if [ "x$status" != "xOK" ] && [ "x$status" != "xWARNING" ]
    then
        msg[m++]="$module: $info"
    fi
    case $status in
        NOTSET)
            setlevel 1;;
        OK)
            setlevel 0;;
        WARNING)
            if [ "$info" != "WARNING; smart: attribute Airflow_Temperature_Cel failed in the past" ] && [ "$info" != "WARNING; smart: attribute Airflow_Temperature_Cel failed" ] && [ "$info" != "WARNING; smart: attribute Airflow_Temperature_Cel failed; smart: attribute Airflow_Temperature_Cel failed in the past" ]
            then
                msg[m++]="$module: $info"
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

die
