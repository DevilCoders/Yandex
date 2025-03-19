#!/bin/bash
export DBUSER=postgres
export PROXY_HOST=s3proxy
export PROXY_DBNAME=s3db
export PROXY_FAKE_FILE=tests/docker.sql
export DB01_DBNAME=s3db
export DB01_HOST=s3db01
export DB01_RO_HOST=s3db01r
export DB02_DBNAME=s3db
export DB02_HOST=s3db02
export META01_DBNAME=s3meta
export META01_HOST=s3meta01
export META02_DBNAME=s3meta
export META02_HOST=s3meta02
export PGMETA_HOST=pgmeta
export PGMETA_DBNAME=s3db
if [ "$1" = 'bash' ]
then
    /bin/bash
elif [ "$1" = 'check' ]
then
    cd /pg/pgproxy/s3db \
    && make
else
    eval "$@"
fi
