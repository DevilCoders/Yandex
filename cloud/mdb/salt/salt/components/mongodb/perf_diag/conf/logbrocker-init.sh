#!/bin/sh
set -x
# no test
for env in prod
do
    for db in mongodb_profiler
    do
        echo "topic: /mdb/porto/${env}/perf_diag/${db}"
        logbroker  -s logbroker schema create topic \
            /mdb/porto/${env}/perf_diag/${db} \
            --abc-service internalmdb \
            --retention-period-sec 129600 \
            -p 2 \
            --responsible "mdb@" \
            -y
    done
done
# no preprod
for env in prod
do
    for db in mongodb_profiler
    do
        echo "topic: /mdb/compute/${env}/perf_diag/${db}"
        logbroker  -s logbroker schema create topic \
            /mdb/compute/${env}/perf_diag/${db} \
            --abc-service dbaas \
            --retention-period-sec 129600 \
            -p 2 \
            --responsible "mdb@" \
            -y
    done
done

