#!/bin/bash

mkdir -p logs
for container in s3db01 s3db02 s3meta01 s3proxy pgmeta
do
  docker logs s3db_${container}_1 > logs/${container}.log 2>&1
  docker exec s3db_${container}_1 cat /tmp/errors > logs/${container}-critical_errors.log
done

tail -n 19 logs/jepsen.log
exit 1
