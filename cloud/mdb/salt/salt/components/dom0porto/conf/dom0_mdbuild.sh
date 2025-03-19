#!/bin/bash

size_blacklist=""
for dev in /dev/sd*
do
    size=$(blockdev --getsize64 "${dev}")
    if [ "$?" != "0" ]
    then
        echo "Filtering out ${dev}: Unable to get size"
        size_blacklist="$dev $size_blacklist"
        continue
    fi
    if [ "$size" -lt 10995116277760 ]
    then
        size_blacklist="$dev $size_blacklist"
    fi
done

used_blacklist=""
for part in $(lsblk /dev/sd* | grep part | grep ^sd | awk '{print $1}')
do
    if grep -q $part /proc/mdstat
    then
        used_blacklist="/dev/$part $used_blacklist"
    fi
done

for dev in $(ls /dev/sd* | grep -v -E '[0-9]+')
do
    if lsblk "$dev" | grep -q part
    then
        used_blacklist="$dev$ $used_blacklist"
    fi
done

blacklist="$(echo $used_blacklist $size_blacklist | xargs -n 1 echo | sort -u | xargs echo | sed 's/\ /|/g')"

if [ "$blacklist" = "" ]
then
    blacklist='^$'
fi

if [ "$1" = "check" ]
then
    unused=$(ls /dev/sd* | grep -cv -E "$blacklist")
    if [ "$unused" -gt 1 ]
    then
        exit 0
    else
        exit 1
    fi
fi

mode=dev
for dev in $(ls /dev/sd* | grep -v -E "$blacklist")
do
    if echo "$dev" | grep -q -E '[0-9]'
    then
        mode=part
        break
    fi
done

set -xe
target="$(( "$(ls /dev/md* | sed 's/\/dev\/md//g' | sort -g | tail -n1)" + 1 ))"

if [ ${target} -lt 100 ]
then
    target=100
fi

while read -r pair
do
    if [ "$(echo ${pair} | wc -w)" != "2" ]
    then
        break
    fi

    list=""
    for dev in $pair
    do
        if [ "$mode" = "dev" ]
        then
        parted "${dev}" mklabel gpt
        parted "${dev}" mkpart primary 0% 100%
        while ! ls "${dev}"1 >/dev/null 2>/dev/null
        do
            partprobe "${dev}"
        done
        list="${dev}1 $list"
    else
        list="${dev} $list"
    fi
    done

    yes | mdadm --create "/dev/md${target}" --force --level=1 --raid-devices=2 $list
    mkfs.ext4 -E lazy_itable_init=0,lazy_journal_init=0 -m0 "/dev/md${target}"
    tune2fs -r 8192 "/dev/md${target}"
    dev_id="$(blkid -s UUID -o export /dev/md${target} | grep UUID)"
    path="/disks/$(echo ${dev_id} | cut -d= -f2)"
    mkdir -p "$path"
    echo "$dev_id $path ext4 defaults,noatime,errors=remount-ro 0 0" >> /etc/fstab
    mount "${path}"
    cat <<EOF > /etc/mdadm/mdadm.conf
CREATE owner=root group=disk mode=0660 auto=yes
HOMEHOST <system>
MAILADDR root
$(mdadm --examine --scan)
EOF
    update-initramfs -u -k all
    target=$(( target + 1 ))
done < <(ls /dev/sd* | grep -v -E "$blacklist" | xargs -L 2 echo)
