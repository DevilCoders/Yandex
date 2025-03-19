#!/bin/bash

PATH="${PATH}:/usr/sbin:/usr/bin:/sbin:/bin:/usr/local/sbin:/usr/local/bin"

dry_run="yes"
if [ $1 == "-n" ]; then
    shift
else
    dry_run="no"
fi

cmd=$@
disk=$1
name=$2
problem_code=$3
shelf=$4
slot=$5
partition_list=$6
disk_type=$7
serial=$8

ctm="date +'%Y/%m/%d %H:%M:%S'"
log() {
    echo -e "$(eval $ctm)\t$@"
}

die() {
    if [ $dry_run == "no" ]; then
        if [ $1 -eq 101 ]; then
            if [ ! -e /var/tmp/hw-watcher-hook-retry ]; then
                touch /var/tmp/hw-watcher-hook-retry
            fi
        else
            rm -f /var/tmp/hw-watcher-hook-retry
            if [ -f /var/tmp/hw-watcher-failed-drive ]; then
                failed_serial=$(cat /var/tmp/hw-watcher-failed-drive)
                python -c 'import json
import sys
try:
    with open("/etc/monitoring/shelves-check-ignore-serials", "r") as f:
        serials=json.load(f)
except:
    serials=[]
serial="'"$failed_serial"'"
if serial in serials:
    serials.remove(serial)
with open("/etc/monitoring/shelves-check-ignore-serials", "w") as f:
    json.dump(serials, f)
'
            fi
        fi
    fi
    log "exit $1"
    exit $1
}

dryrun () {
    if [ $dry_run == "no" ]; then
        "$@"
    else
        log "dry run: $@"
        true # set the $? to 0
    fi
}

my_hostname=`hostname -f`

log "run $0, dry run = $dry_run"
log "'{disk}' '{name}' '{problem_code}' '{shelf}' '{slot}' '{partition_list}' '{disk_type}' '{serial}'"
log "$cmd"

if [ "$partition_list" != "None" ]; then
    log "Partition list is $partition_list, must be system disk. Recovery not needed."
    die 0
fi

rootdir=`readlink -f $(grep root_dir /etc/elliptics/elliptics.conf | sed -E -e '/^#/d' -e 's/.*= *//')`
if [ -z "$rootdir" ]; then
    log "Unable to detect rootdir"
    die 1
fi

detect_mountpoint() {
    empty_mountpoints=()
    mp_list=("$rootdir"/[0-9]*)
    if [ $shelf == "None" ]; then
        if grep -v '^#' /etc/fstab | grep -q '[ \t]/cache'; then
            mp_list+=("/cache")
        fi
        if grep -v '^#' /etc/fstab | grep -q '/srv/storage/cache'; then
            storage_cache_mp="$(grep -v '^#' /etc/fstab | grep -Po '/srv/storage/cache\S*')"
            mp_list+=($storage_cache_mp)
        fi
    fi
    for mp in ${mp_list[*]}; do
        mounted_disk="$(grep -w "$mp" /proc/mounts | awk '{print $1}')"
        if [ -z $mounted_disk ]; then
            empty_mountpoints+=("$mp")
        else
            count="$(echo "$mounted_disk" | wc -l)"
            if [ $count -ne 1 ]; then
                log "More than one drive mounted to $mp:\n$mounted_disk"
                die 1
            fi
            if [ ! -b $mounted_disk ]; then
                empty_mountpoints+=("$mp")
            fi
        fi
    done
    {%- raw %}
    if [ ${#empty_mountpoints[*]} -gt 1 ]; then
        log "Too many mountpoints without drive: ${empty_mountpoints[*]}"
        die 9
    fi
    {% endraw -%}
    echo ${empty_mountpoints[*]}
}

current_mount=$(grep -E "${disk}(n1)? " /proc/mounts | awk '{print $2}')
if [ -n "$current_mount" ]; then
    log "$disk with serial $serial is already mounted to $current_mount. Help me!"
    die 2
fi

if [ -f /var/tmp/hw-watcher-mountpoint-restored ]; then
    mountpoint=$(cat /var/tmp/hw-watcher-mountpoint-restored)
else
    mountpoint=$(detect_mountpoint)
fi

if [ -z $mountpoint ]; then
    log "Mountpoint for $drive cannot be found. Help me!"
    die 10
fi

log "Drive $disk with serial $serial restoring to mountpoint $mountpoint"

uuid=`fgrep -w "$mountpoint" /etc/fstab | grep 'UUID=' | awk '{sub("#*UUID=",""); print $1;}'`
uuidopt=""
if [ -z "$uuid" ]; then
    log "Cannot find UUID for $disk with serial $serial"
    die 1
fi

dryrun sed -i "/$uuid/s/^#*//" /etc/fstab
uuidopt="-U $uuid"

case $mountpoint in
    "/cache"|/srv/storage/cache*)
        log "Refused to use /cache or /srv/storage/cache mount point."
        # Remove mountpoint dir in order not to find it in detect_mountpoint
        rmdir $mountpoint
        die 0
        ;;
    "$rootdir"*)
        if [ $disk_type == "SSD" ]; then
            reserved_space=10
        else
            reserved_space=0
        fi
        ;;
    *)  log "Error: unknown mountpoint $mountpoint for device $disk with serial $serial"
        die 3
esac

# For NVMe we should append namespace number to raw device name
if [[ $disk == /dev/nvme* ]]; then
    disk="${disk}n1"
fi

log "run mkfs.ext4 -F -m$reserved_space $uuidopt $disk with serial $serial"
if [ $dry_run == "no" ]; then
    if ! dryrun mkfs.ext4 -F -m$reserved_space $uuidopt $disk > /dev/null 2>/dev/null
    then
        log "mkfs failed with code $?!"
        die 1
    fi
fi

if ! dryrun mount $disk
then
    log "unable to mount drive $disk with serial $serial: $?!"
    die 1
fi

log "Call masterwrap.py - remove future backends started. masterwrap log: /var/log/hw_watcher/elliptics.log"
log "/usr/lib/yandex/elliptics_hw_watcher/masterwrap.py --remove_future_backend \"$mountpoint\""
dryrun /usr/lib/yandex/elliptics_hw_watcher/masterwrap.py --remove_future_backend "$mountpoint"
code="$?"
case $code in
    101) die 101;;
    0) log "Remove future backends done";;
    *) log "Remove future backends failed. Read /var/log/hw_watcher/elliptics.log"
       die $code
       ;;
esac

log "removing stop files: `grep -Pl "$mountpount(\/|$)" /etc/elliptics/parsed/*.stop 2>/dev/null | xargs`"
dryrun eval "grep -Pl \"$mountpoint(\/|$)\" /etc/elliptics/parsed/*.stop 2>/dev/null | xargs rm -f"

log "Run: create_group_ids.py -m $mountpoint"
dryrun eval "create_group_ids.py -m $mountpoint 2>/dev/null"
case $? in
    101) die 101;;
    0) log "create_group_ids.py done";;
    *) log "create_group_ids.py. Read /var/log/create_group_ids.log"
       die 3
       ;;
esac
sleep 10
timetail -t java -n 300 /var/log/create_group_ids.log | grep -q 'ERROR'
if [ $? -eq 0 ]
then
    log "create_group_ids error;"
    die 4
fi

log "Run: ubic reload elliptics"
dryrun ubic reload elliptics
sleep 10
bc=`ls -1 $mountpoint |grep -v 'lost+found' | wc -l`
backends=(`python -c "import json; conf = json.load(open('/etc/elliptics/parsed/elliptics-node-1.parsed')); backends = [b['backend_id'] for b in conf['backends'] if '$mountpoint/' in b['history']]; print ' '.join(str(p) for p in backends) "`)
{%- raw %}
if [ "x${#backends[@]}" != "x$bc" ]; then
    log "Device backends ($bc) != config backends (${#backends[@]})"
    die 6
fi
{% endraw %}
for b in ${backends[*]}
do
    log "Run: sa-dnet start $b"
    dryrun sa-dnet start $b
done

dryrun rm /var/tmp/hw-watcher-mountpoint-restored /var/tmp/hw-watcher-failed-drive
die 0
