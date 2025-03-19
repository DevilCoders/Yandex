#!/usr/bin/env bash

logs_dir="prod_sas_logs_20210617"
suffix=".4"

#rm -rf $logs_dir &&
#echo "copying cluster logs to ${logs_dir}" &&
#pssh scp -p 50 "C@cloud_prod_nbs_sas:/Berkanavt/nbs-server/log/nbs_start.log${suffix}.gz ./$logs_dir &&
#pssh scp -p 50 "C@cloud_prod_nbs_sas:/Berkanavt/nbs-server/log/nbs.log${suffix}.gz ./$logs_dir &&
echo "processing logs" &&
rm -f out.txt &&
rm -f err.txt &&
ls $logs_dir | while read line;
do
    start_log="${logs_dir}/${line}/nbs_start.log${suffix}"
    main_log="./${logs_dir}/${line}/nbs.log${suffix}"

    if [ -f "${start_log}.gz" ] && [ -f "${main_log}.gz" ];
    then
        echo "$line" >> err.txt &&
        echo "$line" >> out.txt &&
        cat "${start_log}.gz" | gunzip > "$start_log" &&
        cat "${main_log}.gz" | gunzip > "$main_log" &&

        $(./process-startup-events "$start_log" "$main_log" 2>> err.txt >> out.txt;) || echo "failed to process ${line} logs"
    fi
done
