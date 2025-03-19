#!/bin/bash

python3 ./shifts.py mdbcoreteam service_duty ./configs/core_resps.yml || exit 1
python3 ./shifts.py dataprocessing watch ./configs/dataproc_resps.yml || exit 1
python3 ./shifts.py gendb gendb_duty ./configs/gendb_resps.yml || exit 1
python3 ./shifts.py nonrelational_dbs nonrelational_dbs_duty_2022 ./configs/nonrelational_dbs_resps.yml || exit 1
python3 ./shifts.py analytic_dbs duty ./configs/analytic_dbs_resps.yml || exit 1
python3 ./shifts.py esaas mdb_es_service_duty ./configs/elasticsearch_dbs_resps.yml || exit 1
python3 ./shifts.py mm mm ./configs/mm_resps.yml || exit 1

for file in hosts/*
do
    retry=1
    tries=0
    while [[ "$retry" != "0" ]] && [[ "$tries" -lt 3 ]]
    do
        echo "Applying file $file"
        ansible-playbook -i inventory "$file" -vvv
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

