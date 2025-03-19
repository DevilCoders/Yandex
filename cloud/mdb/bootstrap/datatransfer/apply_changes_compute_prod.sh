#!/bin/bash -e

which ya >/dev/null || { echo "ya utility was not found in \$PATH ($PATH)"; exit 1; }

echo "Obtain IAM token using yc: yc iam create-token"
read -r -p "Enter token: " TOKEN
test -z "${TOKEN}" && { echo "Need token to continue"; exit 1; }
export TOKEN=${TOKEN}

for service in postgres mysql specdb kafka greenplum mysql_perf_diag postgres_perf_diag
do
    echo "Processing ${service} ..."
    python3 cli.py --conf config/compute-prod/${service}.yaml create
    python3 cli.py --conf config/compute-prod/${service}.yaml start
    python3 cli.py --conf config/compute-prod/${service}.yaml drop-orphans
done
