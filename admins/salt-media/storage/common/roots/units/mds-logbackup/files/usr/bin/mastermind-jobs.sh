#!/bin/bash
max_stale=21
max_stale_list=60

path='/var/log/mastermind/jobs'
if [ -e $path ]
then
    cd $path
    # archive
    dir=`find * -maxdepth 0 -type d -mtime +$max_stale |grep -v lrc_list`

    for d in $dir
    do
        if [ `ps aux | grep $d | grep -v grep | wc -l` -eq 0 ]
        then
            tar --remove-files --force-local --atime-preserve -I zstd -cf "$d-`date '+%Y%m%d-%s'`.tar.zst"  $d
        fi
    done

    dir=`find * -maxdepth 0 -type d -mtime +$max_stale_list |grep lrc_list`

    for d in $dir
    do
        if [ `ps aux | grep $d | grep -v grep | wc -l` -eq 0 ]
        then
            tar --remove-files --force-local --atime-preserve -I zstd -cf "$d-`date '+%Y%m%d-%s'`.tar.zst"  $d
        fi
    done
fi
