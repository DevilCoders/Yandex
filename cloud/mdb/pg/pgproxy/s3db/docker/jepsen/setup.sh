#!/bin/bash
export DBUSER=postgres
export DB01_DBNAME=s3db
export DB01_HOST=s3db01
export DB02_DBNAME=s3db
export DB02_HOST=s3db02
export META01_DBNAME=s3meta
export META01_HOST=s3meta01
export PGMETA_HOST=pgmeta
export PGMETA_DBNAME=s3db
export PROXY_HOST=s3proxy
export PROXY_DBNAME=s3db
export PROXY_FAKE_FILE=tests/docker.sql

cd /pg/pgproxy/s3db && make deploy-jepsen && make prepare-jepsen
