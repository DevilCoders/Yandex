#!/bin/bash

for i in 1 2 3
do
    mkdir -p logs/postgresql${i}
    mkdir -p logs/zookeeper${i}
    for service in pgbouncer gpsync
    do
        docker exec gpsync_postgresql${i}_1 cat \
            /var/log/${service}.log > \
            logs/postgresql${i}/${service}.log
    done
    docker exec gpsync_postgresql${i}_1 cat \
        /var/log/postgresql/postgresql.log > \
        logs/postgresql${i}/postgresql.log
    docker exec gpsync_zookeeper${i}_1 cat \
        /var/log/zookeeper/zookeeper--server-gpsync_zookeeper${i}_1.log > \
        logs/zookeeper${i}/zk.log 2>&1
done

tail -n 18 logs/jepsen.log
# Explicitly fail here
exit 1
