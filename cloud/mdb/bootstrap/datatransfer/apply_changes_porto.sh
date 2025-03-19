#!/bin/bash -e

which ya >/dev/null || { echo "ya utility was not found in \$PATH ($PATH)"; exit 1; }

echo "Obtain internal Passport token here:"
echo "https://oauth.yandex-team.ru/authorize?response_type=token&client_id=8cdb2f6a0dca48398c6880312ee2f78d"
read -r -p "Enter token: " TOKEN
test -z "${TOKEN}" && { echo "Need token to continue"; exit 1; }
export TOKEN=${TOKEN}

for service in postgres mysql specdb postgres_perf_diag postgres_perf_diag_rt mysql_perf_diag mysql_perf_diag_rt kafka greenplum
do
    echo "Processing ${service} ..."
    python3 cli.py --conf config/porto/${service}.yaml create
    python3 cli.py --conf config/porto/${service}.yaml start
    python3 cli.py --conf config/porto/${service}.yaml drop-orphans
done
