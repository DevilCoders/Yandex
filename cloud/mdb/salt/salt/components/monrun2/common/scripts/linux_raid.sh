#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin

die() {
    echo "$1;$2"
    exit 0
}

stat_file="/proc/mdstat"
mdadm="sudo -n mdadm --detail"
conf_files="/etc/mdadm/mdadm.conf /etc/mdadm.conf"
hw_watcher_config_file="/etc/hw_watcher/hw_watcher.conf"
err_code=0
message=""

if [ -f /usr/local/sbin/autodetect_environment ] ; then
    is_virtual_host=0
    . /usr/local/sbin/autodetect_environment >/dev/null 2>&1 || true
    if [ "$is_virtual_host" -eq 1 ] ; then
        die 0 "OK"
    fi
fi

#Parse config
arrays=""
for conf_file in $conf_files ; do
    if [ -f "$conf_file" ]; then
        arrays=$(awk '/^\s*ARRAY/ { gsub("/dev/|/","",$2); print $2 }' "$conf_file")
        break
    fi
done

# Also take a look into /proc/mdstat
arrays="${arrays} $(awk '/^md[0-9pd_]+ :/ {print $1}' ${stat_file})"

# Remove duplicates
arrays=$(echo "${arrays}" | sed -e 's/ /\n/g' | sort | uniq)

for md in $arrays
do
    state=""
    # Check if array is active in /proc/mdstat
    state=$(grep "${md} :" ${stat_file} | cut -d' ' -f 3)
    if [ "x${state}" != "xactive" ]; then
        err_code=2
        message="${message} ${md}:${state}"
        continue
    fi

    # Its active alright, take a more thorough look.
    state=$(${mdadm} "/dev/${md}" 2>&1 | awk -F':' '/State.+(FAILED|recovery|degraded)/ {print $2}')
    [ "${state}" ] || continue
    message="${message} ${md}:${state}"

    # Handle rebuild case
    # e.g.: Rebuild Status : 64% complete
    rebuild_status=$(${mdadm} "/dev/${md}" 2>&1 | grep 'Rebuild Status :')
    case "${rebuild_status}" in
        *Rebuild*)
            message="${message} at $(echo "$rebuild_status" | awk '{print $4};')"
            # If state is not CRIT, make it WARN beacause of the rebuild
            [ "$err_code" -eq "2" ] || err_code=1
        ;;
    esac

    case "${message}" in
        *FAILED*|*faulty*|*removed*|*degraded*)
            err_code=2
        ;;
        *recovery*)
            [ ${err_code} -lt 2 ] && err_code=1
        ;;
    esac
done

[ "${message}" ] || message="OK"
if [ "${err_code}" -eq "2" ] && [ -f "${hw_watcher_config_file}" ]
then
    err_code=1
fi
die $err_code "${message}"
