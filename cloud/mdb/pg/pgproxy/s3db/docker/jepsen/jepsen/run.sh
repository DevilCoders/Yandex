#!/bin/sh

set -e
set -x

cd "$(dirname "$0")"
export LEIN_ROOT=1
for i in s3db01 s3db02 s3meta01 s3proxy pgmeta
do
    ssh-keyscan -t rsa s3db_${i}_1.s3db_net >> /root/.ssh/known_hosts
done

lein test
