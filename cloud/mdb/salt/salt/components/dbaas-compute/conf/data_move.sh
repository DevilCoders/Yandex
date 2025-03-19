#!/bin/bash

set -xe
export SYSTEMD_PAGER=''

disable_service () {
{% if accumulator | default(False) %}
{%   if 'compute-data-move-disable' in accumulator %}
{%     for line in accumulator['compute-data-move-disable'] %}
{{ line }}
{%     endfor %}
{%   endif %}
{% endif %}
return
}

enable_service () {
{% if accumulator | default(False) %}
{%   if 'compute-data-move-enable' in accumulator %}
{%     for line in accumulator['compute-data-move-enable'] %}
{{ line }}
{%     endfor %}
{%   endif %}
{% endif %}
return
}

BLOCK_SIZE=1M
LOG_FILE=/var/log/data_move.log

if [ $# -lt 1 ]
then
    echo Must set script work mode
    exit 42
fi
DIRECTION=$1

to_log() {
    echo `date +'%Y-%m-%d %H:%M:%S'` ${DIRECTION}_$1 $2 $3 >> $LOG_FILE
}

function get_last() {
    num_lines=`tac $LOG_FILE | grep -m 1 -n -v " ${DIRECTION}_" | cut -f1 -d:`
    if [ "" == "$num_lines" ]
    then
        num_lines=$(/usr/bin/wc -l $LOG_FILE)
    else
        num_lines=$(($num_lines-1))
    fi

    tac $LOG_FILE | head -n $num_lines | grep -m 1 "\<${DIRECTION}_$1\>"
    return 0
}

to_log start

prev_finished=$(get_last finished)
if [ "" != "$prev_finished" ]
then
    to_log finished_already
    exit 0
fi

prev_goal=$(get_last goal)
if [ "" == "$prev_goal" ]
then
    BOOT_DEVICE="$(/bin/mount | /usr/bin/cut -f 1,3 -d ' ' | /bin/grep -E '\ /$' | /usr/bin/cut -d ' ' -f1 | /bin/sed 's/[0-9]*//g')"
    if [ "$DIRECTION" != "back" ]
    then
        SOURCE="$(/bin/mount | /bin/grep ext4 | /usr/bin/cut -f 1,3 -d ' ' | /bin/grep -v ${BOOT_DEVICE} | cut -d ' ' -f1)"
    else
        SOURCE="$(/bin/ls /dev/vd[a-z]*1 | /bin/grep -v ${BOOT_DEVICE})"
        source_count=$(echo $SOURCE | /usr/bin/wc -w)
        if [ "$source_count" != "1" ]
        then
            /bin/echo "Found $source_count sources, candidate devices: '$SOURCE'. Exiting"
            to_log unknown_source $SOURCE
            exit 1
        fi
    fi

    blacklist="${BOOT_DEVICE}[0-9]*|$(/bin/echo ${SOURCE} | /bin/sed 's/[0-9]*//g')[0-9]*"
    if [[ $SOURCE == /dev/md[0-9]* ]]
    then
        blacklist=${blacklist}$(eval mdadm --detail ${SOURCE} | grep -o "/dev/vd.*" | xargs -n 1 -I{} echo -n "|{}")
    fi
    free_devices="$(/bin/ls /dev/vd* | /bin/grep -v -E "$blacklist" | /bin/sed 's/[0-9]*//g' | /usr/bin/sort -u)"
    num_free_devices=$(echo $free_devices | /usr/bin/wc -w)
    if [ "$num_free_devices" = "1" ]
    then
        DATA_DEVICE=$free_devices
        if ! /bin/ls "${DATA_DEVICE}1"
        then
            to_log started_parted_target ${DATA_DEVICE}
            /sbin/parted "${DATA_DEVICE}" mklabel gpt
            /sbin/parted "${DATA_DEVICE}" mkpart primary 0% 100%
            to_log prepare $DATA_DEVICE
        fi
        TARGET="${DATA_DEVICE}1"
    else
        md_devices=$(/bin/ls /dev/md[0-9]* 2> /dev/null; true)
        num_md_devices=$(echo $md_devices | /usr/bin/wc -w)
        if [ "$num_md_devices" != "1" ]
        then
            /bin/echo "Found $num_free_devices target devices: $free_devices but no mdraid. Exiting"
            to_log unknown_target $free_devices $md_devices
            exit 1
        fi
        TARGET=$md_devices
    fi

    if [ "$TARGET" == "$SOURCE" ]
    then
        /bin/echo "Target is same as source! It both: $TARGET"
        to_log unknown_target_for_source $SOURCE
        exit 1
    fi

    to_log goal $SOURCE $TARGET
else
    SOURCE=`echo $prev_goal | cut -f4 -d ' '`
    TARGET=`echo $prev_goal | cut -f5 -d ' '`
fi

if [ "$TARGET" == "$SOURCE" ]
then
    /bin/echo "Target is same as source! It both: " $TARGET
    to_log unknown_target_for_source $SOURCE
    exit 1
fi

if [ "$DIRECTION" != "back" ]
then
    prev_disabled=$(get_last disabled)
    if [ "" == "$prev_disabled" ]
    then
        disable_service
        to_log disabled
    fi

    prev_reduce_size=$(get_last reduce_size)
    if [ "" == "$prev_reduce_size" ]
    then
        src_mb=$(($(/sbin/blockdev --getsz $SOURCE)/2048))
        tgt_mb=$(($(/sbin/blockdev --getsz $TARGET)/2048))
        if [ $src_mb -gt $tgt_mb ]
        then
            to_log started_reduce_e2fsck $SOURCE
            set +e
            /sbin/e2fsck -yf "${SOURCE}"
            check_code=$?
            set -e
            if [ $check_code -gt 1 ]
            then
                /bin/echo e2fsck failed with unsafe code $check_code
                to_log reduce_e2fsck_failed $SOURCE $check_code
                exit $check_code
            fi

            to_log started_reduce_size ${src_mb}M ${tgt_mb}M
            /sbin/resize2fs "${SOURCE}" ${tgt_mb}M 2>&1
            /bin/sync
            to_log reduce_size finished
        else
            to_log reduce_size not required
        fi
    fi
fi

prev_moved=$(get_last moved)
if [ "" == "$prev_moved" ]
then
    # should processed in salt state data_move
    apt-get -qq -y install partclone zstd

    to_log started_move $SOURCE $TARGET
    if [ "$DIRECTION" == "front" ]
    then
        /usr/sbin/partclone.ext4 -c -d -s $SOURCE | /usr/bin/zstd - -T0 | /bin/dd of=$TARGET bs=$BLOCK_SIZE
        move_result=$((${PIPESTATUS[0]}+${PIPESTATUS[1]}+${PIPESTATUS[2]}))
    elif [ "$DIRECTION" == "back" ]
    then
        /bin/dd if=$SOURCE bs=$BLOCK_SIZE | /usr/bin/zstd -d -T0 | /usr/sbin/partclone.ext4 -r -s - -o $TARGET
        move_result=${PIPESTATUS[2]}
    else
        set +e
        /usr/sbin/partclone.ext4 -b -s $SOURCE -o $TARGET
        set -e
        move_result=$?
    fi

    if [ $move_result -ne 0 ]
    then
        /bin/echo data move failed with code $move_result
        to_log failed_move $move_result
        exit $move_result
    fi
    /bin/sync
    to_log moved
fi

if [ "$DIRECTION" != "front" ]
then
    to_log started_goal_resize $TARGET
    prev_goal_resized=$(get_last goal_resized)
    if [ "" == "$prev_goal_resized" ]
    then
        to_log started_goal_e2fsck $TARGET
        set +e
        /sbin/e2fsck -yf "${TARGET}"
        check_code=$?
        set -e
        if [ $check_code -gt 1 ]
        then
            /bin/echo e2fsck goal partition failed with unsafe code $check_code
            to_log goal_e2fsck_failed $TARGET $check_code
            exit $check_code
        fi

        /sbin/resize2fs "${TARGET}"
        /bin/sync
        to_log goal_resized $TARGET
    fi

    # enable /var/lib/... partition in /etc/fstab
    sed -i '/^#UUID=[0-9a-f-]* \/var\/lib\//s/^#//' /etc/fstab

    prev_enabled=$(get_last enabled)
    if [ "" == "$prev_enabled" ]
    then
        to_log started_enable
        enable_service
        to_log enabled
    fi
else
    # disable /var/lib/... partition in /etc/fstab
    sed -i 's/^UUID=[0-9a-f-]* \/var\/lib\//#&/' /etc/fstab

    # workaround to avoid two ipv6 addresses on new instance (old ipv6 and new ipv6 )
    rm -f /var/lib/dhcp/dhclient6.eth1.leases
fi

to_log finished
