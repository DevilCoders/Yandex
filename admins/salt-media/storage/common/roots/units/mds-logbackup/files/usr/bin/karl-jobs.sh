#!/bin/bash
max_stale=7

path='/var/log/karl/jobs'
if [ -e $path ]
then
    cd $path
    dir=$(find . -maxdepth 1 -type d -mtime +$max_stale | grep -o "KARL.*")
    for d in $dir
    do
        if [[ ! $(karl-cli job status --tls $d 2>/dev/null) || $(karl-cli job status --tls $d 2>/dev/null | grep status | awk '{ print $2 }') == "Finished" ]]
        then
            tar --remove-files --force-local --atime-preserve -I zstd -cf "$d-$(date '+%Y%m%d-%s').tar.zst"  $d
        fi
    done
fi
