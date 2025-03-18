#!/bin/sh
for i in "ru" "ua" "by" "kz" "tr" "id" "mobile" ; do
    ./serpmetrics.py -c ./cfg/config_${i}.py -j 8 >>nohup.out &
done

#delete old src files
find ./ -name "*.gz" -mtime +180 -type f -delete
