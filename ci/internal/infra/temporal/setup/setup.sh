#!/bin/bash
set -e

TEMPORAL_VERSION="1.16.2"

while getopts ":e:" opt; do
    case "$opt" in
        e)  ENV=$OPTARG ;;
        \?)  echo "Invalid option '-$OPTARG'" >&2 && exit 42 ;;
        :)  echo "Option '-$OPTARG' requires an argument." >&2 && exit 42 ;;
    esac
done

if [ -z "$ENV" ] ; then
    echo 'Missing environment (-e)' >&2
    exit 42
fi

source ../configure-env.sh

# See https://github.com/temporalio/temporal/blob/master/docker/auto-setup.sh for docker params
# Also see https://github.com/temporalio/temporal/issues/2293 and https://github.com/temporalio/temporal/blob/master/tools/sql/main.go#L110

COMMON_VARS="-e SKIP_DEFAULT_NAMESPACE_CREATION=true"
PG_VARS="-e SKIP_DB_CREATE=true -e DB=postgresql  -e POSTGRES_SEEDS=${DB_HOST} -e POSTGRES_USER=${DB_USER} -e POSTGRES_PWD=${DB_PASSWORD} -e DB_PORT=${DB_PORT} -e SQL_TLS=true -e SQL_TLS_DISABLE_HOST_VERIFICATION=true -e SQL_CONNECT_ATTRIBUTES='binary_parameters=yes'"
ELASTIC_VARS="-e ENABLE_ES=true -e ES_SCHEME=https -e ES_SEEDS=${ELASTIC_HOST} -e ES_USER=${ELASTIC_USER} -e ES_PWD=${ELASTIC_PASSWORD}"
ALL_VARS="${COMMON_VARS} ${PG_VARS} ${ELASTIC_VARS}"

docker build -t temporal-auto-setup --build-arg TEMPORAL_VERSION=${TEMPORAL_VERSION} .
CMD="docker run -i --rm --network host --name temporal-auto-setup ${ALL_VARS} temporal-auto-setup"
echo "Running: '$CMD'"
eval $CMD

