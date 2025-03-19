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
dmesg_flag=$9

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
rootdir=`readlink -f $(grep root_dir /etc/elliptics/elliptics.conf | sed -E -e '/^#/d' -e 's/.*= *//')`
if [ -z $rootdir ]; then
    log "Unable to detect rootdir"
    die 1
fi
set_downtime () {
    if [ $dry_run == "yes" ]; then return; fi
    if [ -z $2 ]
    then
        time=259200
    else
        time=$2
    fi
    /usr/bin/sa-downtime -s "$my_hostname $1 ${time}"
    # Set downtime for shelves-check by drive serial number
    python -c 'import json
import sys
try:
    with open("/etc/monitoring/shelves-check-ignore-serials", "r") as f:
        serials=json.load(f)
except:
    serials=[]
serial="'"$serial"'"
if not serial in serials:
    serials.append(serial)
with open("/etc/monitoring/shelves-check-ignore-serials", "w") as f:
    json.dump(serials, f)
'

    # Remember which drive we are recovering right now
    echo $serial > /var/tmp/hw-watcher-failed-drive
}

detect_mountpoint() {
    if [ "$disk" != "None" ]; then
        if [[ "$disk" == /dev/nvme* ]]; then
            disk="${disk}n1"
        fi
        mountpoint="$(grep "$disk " /proc/mounts | awk '{print $2}')"
        if [ -n "$mountpoint" ]; then
            count="$(echo "$mountpoint" | wc -l)"
            if [ $count -gt 1 ]; then
                log "Disk $disk mounted $count times"
                die 1
            fi
            echo "$mountpoint"
            return
        fi

        uuid="$(blkid -o value -s UUID $disk)"
        if [ -n "$uuid" ]; then
            mountpoint="$(grep "UUID=$uuid" /etc/fstab | awk '{print $2}')"
            count="$(echo "$mountpoint" | wc -l)"
            if [ -n "$mountpoint" ]; then
                if [ $count -gt 1 ]; then
                    log "Error: more than one fstab entry for UUID $uuid"
                    die 1
                fi
                echo "$mountpoint"
                return
            fi
        fi
        # Mountpoint still cannot be detected, fallback to missing drive
    fi
    # Check all mountpoints we know about
    empty_mountpoints=()
    mp_list=("$rootdir"""/[0-9]*)
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

log "run $0, dry run = $dry_run"
log "'{disk}' '{name}' '{problem_code}' '{shelf}' '{slot}' '{partition_list}' '{disk_type}' '{serial}' '{dmesg_flag}'"
log "$cmd"

if [ -e /var/tmp/watcher-force-disable ]
then
    log "Force replace disk: $disk with serial $serial"
    dryrun rm /var/tmp/watcher-force-disable
    set_downtime "hw_errs,raid,shelves-check,filesystem-clean,elliptics-dnet"
    die 0
fi

case "$problem_code" in
    '0') log "problem_code=$problem_code.";
         set_downtime "hw_errs,raid"
         die 0;
         ;;
    '2') log "problem_code=$problem_code. Help me!";
         die 1;
         ;;
esac

# For NVMe we should append namespace number to raw device name
if [[ $disk == /dev/nvme* ]]; then
    disk="${disk}n1"
fi

mountpoint_candidate=$(detect_mountpoint "$disk")
if [ "$disk" == "None" -a "$shelf" != "None" ]
then
    # This is not protected with dry_run because it shouldn't reset a healthy disk
    log "/usr/bin/st-reset-sas-phy.sh ${shelf} ${slot}"
    /usr/bin/st-reset-sas-phy.sh ${shelf} ${slot}
    reset_status="$?"
    if [ $reset_status -eq 0 ]
    then
        log 'SAS phy reset'
        die 101
    fi
    log "SAS phy reset status: $reset_status"
    log 'Disk found in bot, but not detected by system'
fi

set_downtime "hw_errs,raid,shelves-check,filesystem-clean,elliptics-dnet" 43200

if [ -f /var/tmp/hw-watcher-mountpoint-$serial ]; then
    mountpoint=$(cat /var/tmp/hw-watcher-mountpoint-$serial)
    if [ "$mountpoint" != "$mountpoint_candidate" ]; then
        log "Previous known mountpoint for $serial: $mountpoint is different from current: $mountpoint_candidate, must be the same"
        die 1
    fi
else
    if [ $dry_run == "no" ]; then
        echo $mountpoint_candidate > /var/tmp/hw-watcher-mountpoint-$serial
    fi
    mountpoint=$mountpoint_candidate
fi
if [ -z $mountpoint ]
then
    log "Unable to find mountpoint for $disk with serial $serial"
    die 1
else
    log "$disk with serial $serial is mounted to $mountpoint"
fi

lock="/var/lock/hw_disk_replace.lock"

case "$mountpoint" in
    "$rootdir"/cache*)
        backends=`python -c "import json; conf = json.load(open('/etc/elliptics/parsed/elliptics-node-1.parsed')); backends = [\"{0}:{1}\".format(int(b['backend_id']),int(b['group'])) for b in conf['backends'] if '/srv/storage/cache/1/' in b['history']]; print ' '.join(str(p) for p in backends) "`
        log Backends: $backends
        for bg in $backends; do
            b=${bg/:*/}
            g=${bg/*:/}
            dryrun sa-dnet remove $b
            log /usr/lib/yandex/elliptics_hw_watcher/masterwrap.py -d "/srv/storage/cache/1/" -g $g
            dryrun /usr/lib/yandex/elliptics_hw_watcher/masterwrap.py -d "/srv/storage/cache/1/" -g $g || die 101
        done
        sleep 30
        dryrun umount $mountpoint
        dryrun rm -f /var/tmp/hw-watcher-mountpoint-$serial
        dryrun eval "echo \"$mountpoint\" > /var/tmp/hw-watcher-mountpoint-restored"
        die 0
        ;;
    "$rootdir"*)
        uuid=`fgrep -w $mountpoint /etc/fstab | awk '{sub("#?UUID=",""); print $1;}'`
        masterwrap_lock="masterwrap_{{ grains['conductor']['root_datacenter'] }}"
        # echo "hw_watcher: started backup jobs for `hostname -f`" | mail -s "Elliptics-hw_watcher backup job for `hostname -f`:$mountpoint" mds-cc@yandex-team.ru
        log "Call masterwrap.py - backup started. masterwrap log: /var/log/hw_watcher/elliptics.log"
        log "/usr/lib/yandex/elliptics_hw_watcher/masterwrap.py -r \"$mountpoint/*/\" -i $serial"
        dryrun zk-flock ${masterwrap_lock} -n 10 -w 300 -x 103 "/usr/lib/yandex/elliptics_hw_watcher/masterwrap.py -r '$mountpoint/*/' -i $serial"
        code="$?" 
        case $code in
            101) rm /var/tmp/hw-watcher-mastermind-fail-$serial
                 die 101
                 ;;
            103) rm /var/tmp/hw-watcher-mastermind-fail-$serial
                 log "Failed to get lock ${masterwrap_lock}"
                 die 101
                 ;;
            4|8) if ! [ -f /var/tmp/hw-watcher-mastermind-fail-$serial ]; then
                     touch /var/tmp/hw-watcher-mastermind-fail-$serial
                 fi
                 die 101
                 ;;
            0) rm /var/tmp/hw-watcher-mastermind-fail-$serial
               log "Backup done"
               ;;
            *) log "Backup failed. Read /var/log/hw_watcher/elliptics.log"
               die $code
               ;;
        esac
        
        if [ ! -z $uuid ]; then
            dryrun sed -i "/$uuid/s/^#*/#/" /etc/fstab
        fi

        if grep -q "$mountpoint " /proc/mounts; then
            log "umount $mountpoint"
            dryrun umount $mountpoint
            if [ $? -ne 0 ]
            then
                log "umount -l $montpoint"
                dryrun umount -l $mountpoint
            fi
        fi

        dryrun rm -f /var/tmp/hw-watcher-mountpoint-$serial
        dryrun eval "echo \"$mountpoint\" > /var/tmp/hw-watcher-mountpoint-restored"
        if [ $dmesg_flag -eq 1 ]
        then
            # MDS-18605
            log "Exit without ITDC ticket"
            die 102
        else
            die 0
        fi 
        ;;
    /cache)
        if mount | grep -q /place/cocaine-isolate-daemon/ssd4app; then
            # obsolete configuration
            dryrun ubic stop cocaine-runtime
            dryrun ubic stop cocaine-isolate-daemon
            dryrun portoctl gc
            dryrun eval "timeout 300 service yandex-porto stop || true"
            dryrun umount /place/cocaine-isolate-daemon/ssd4app
            dryrun "timeout 300 service yandex-porto start || true"
            dryrun ubic start cocaine-isolate-daemon
            dryrun ubic start cocaine-runtime
        fi


        if grep datasort_dir /etc/elliptics/parsed/elliptics-node-1.parsed | grep -q '/cache/defrag'; then
            for backend in $(elliptics-node-info.py | grep "defrag_state=RUN" | awk '{print $1}' | cut -d = -f 2); do
                log "Stopping defrag for backend $backend"
                if ! dryrun sa-dnet stop_defrag $backend; then
                    log "Unable to stop defrag for backend $backend"
                    die 10
                fi
            done
        fi

        if [ -d /cache/defrag ]; then
            # Fix for situations wher SSD slows down to 1 MB/sec
            dryrun mv /cache/defrag /cache/defrag_
        fi

        if [ $(lsof /cache | wc -l) -gt 0 ]; then
            log "Some processes are still using /cache"
            die 101
        fi

        if [ -d /cache/defrag ]; then
            dryrun rm -rf /cache/defrag
        fi

        if [ -d /cache/defrag_ ]; then
            dryrun rm -rf /cache/defrag_
        fi

        if ! dryrun umount /cache ; then
            log "Unable to umount /cache"
            die 11
        fi
        if [ -d /cache/defrag ]; then
            # Remove /cache/defrag on root fs also
            dryrun rm -rf /cache/defrag
        fi
        dryrun rm -f /var/tmp/hw-watcher-mountpoint-$serial
        dryrun eval "echo "$mountpoint" > /var/tmp/hw-watcher-mountpoint-restored"
        die 0
        ;;
    *)
        log "Disk $disk with serial $serial is not OK to replace"
        die 44
        ;;
esac

die 101
