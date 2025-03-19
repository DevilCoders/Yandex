#!/bin/bash

python3 ./shifts.py mdbcoreteam service_duty ./configs/core_resps.yml || exit 1
python3 ./shifts.py dataprocessing duty ./configs/dataproc_resps.yml || exit 1
python3 ./shifts.py gendb gendb_duty ./configs/gendb_resps.yml || exit 1
python3 ./shifts.py mm mm ./configs/mm_resps.yml || exit 1

for file in hosts/*
do
    retry=1
    tries=0
    while [[ "$retry" != "0" ]] && [[ "$tries" -lt 3 ]]
    do
        echo "Checking file $file (attempt $tries)"
        ansible-playbook -i inventory -vvv --check "$file"
        retry=$?
        if [ "$retry" != "0" ]
        then
            tries=$(( tries + 1 ))
            sleep 5
        fi
    done
    if [ "$retry" != 0 ]
    then
        exit 1
    fi
done
