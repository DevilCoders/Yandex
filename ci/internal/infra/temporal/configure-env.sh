#!/bin/bash
set -e

case ${ENV} in
    testing)
        GRPC_ADDRESS="dns:///ci-temporal-frontend-testing.in.yandex.net:7233"
        DB_CLUSTER="mdb85p8hbsmrfd7mqgrm"
        ELASTIC_CLUSTER="mdbfsuq75f1t6jhas37e"
        ;;
    prestable)
        GRPC_ADDRESS="dns:///ci-temporal-frontend-prestable.in.yandex.net:7233"
        DB_CLUSTER="mdb01u11b1rni8r4t6rk"
        ELASTIC_CLUSTER="mdborlbd4qt20dsiie54"
        ;;
    stable)
        GRPC_ADDRESS="dns:///ci-temporal-frontend.in.yandex.net:7233"
        DB_CLUSTER="mdbvd729uiu3pvi842m3"
        ELASTIC_CLUSTER="mdbot3l5eg8s2c4qkq5k"
        ;;
    *)
        echo "Unsupported env: '${ENV}'" >&2
        exit 42
esac


DB_HOST="c-${DB_CLUSTER}.rw.db.yandex.net"
DB_PORT="6432"
DB_ADDRESS="${DB_HOST}:${DB_PORT}"
DB_USER="temporal-${ENV}"

ELASTIC_HOST="c-${ELASTIC_CLUSTER}.rw.db.yandex.net"
ELASTIC_PORT="9200"
ELASTIC_ADDRESS="${ELASTIC_HOST}:${ELASTIC_PORT}"
ELASTIC_USER=admin

S3_BUCKET="ci-temporal-${ENV}"

if [ -z "${DB_PASSWORD}" ] ; then
    echo 'DB_PASSWORD env variable must be provided (recommended from secrets) to run this script' >&2
    exit 42
fi

if [ -z "${ELASTIC_PASSWORD}" ] ; then
    echo 'ELASTIC_PASSWORD env variable must be provided (recommended from secrets) to run this script' >&2
    exit 42
fi

export GRPC_ADDRESS

export DB_HOST
export DB_PORT
export DB_ADDRESS
export DB_USER

export ELASTIC_ADDRESS
export ELASTIC_USER

export S3_BUCKET
