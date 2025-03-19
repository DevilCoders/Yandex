#!/usr/bin/env bash

time_now=$((`date +%s`*1000+`date +%-N`/1000000))
time_delta=1800000

time_end=$((time_now - time_delta))

check=`mysql yaroot -e "select * from task_logs where not code = \"10\" and not code = \"11\" and not code = \"20\" and end > \"$time_end\";" | wc -l`

if [[ $check > 0 ]]; then
        echo "2; found $check errors"
else
        echo "0; ok"
fi
