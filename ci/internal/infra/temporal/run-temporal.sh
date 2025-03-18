#!/bin/bash
set -e

checkS3Credentials(){
    if [ -z "${AWS_ACCESS_KEY_ID}" ] ; then
        echo 'AWS_ACCESS_KEY_ID env variable must be provided (recommended from secrets) to run temporal with s3' >&2
        exit 42
    fi

    if [ -z "${AWS_SECRET_ACCESS_KEY}" ] ; then
        echo 'AWS_SECRET_ACCESS_KEY env variable must be provided (recommended from secrets) to run temporal with s3' >&2
        exit 42
    fi
}

TEMPORAL_DIR="/temporal/"

while getopts ":e:s:d:" opt; do
    case "$opt" in
        e)  ENV=$OPTARG ;;
        s)  SERVICE=$OPTARG ;;
        d)  TEMPORAL_DIR=$OPTARG ;;
        \?)  echo "Invalid option '-$OPTARG'" >&2 && exit 42 ;;
        :)  echo "Option '-$OPTARG' requires an argument." >&2 && exit 42 ;;
    esac
done

if [ -z "$ENV" ] ; then
    echo 'Missing environment (-e)' >&2
    exit 42
fi
if [ -z "$SERVICE" ] ; then
    echo 'Missing service (-s)' >&2
    exit 42
fi

echo "ENV = ${ENV}"
echo "SERVICE = ${SERVICE}"
echo "TEMPORAL_DIR = ${TEMPORAL_DIR}"


cd ${TEMPORAL_DIR}

CONFIG="config/${ENV}.yaml"

BIND_ADDRESS="$(hostname --ip-address)"
echo "BIND_ADDRESS = ${BIND_ADDRESS}"

source configure-env.sh

checkS3Credentials

#This magic puts env variable to config
eval "echo \"$(cat config/template.yaml)\"" > ${CONFIG}
echo "Created config ${TEMPORAL_DIR}/${CONFIG}"

echo "Running  ${SERVICE} temporal service"

./temporal-server --env ${ENV} start --service ${SERVICE}

