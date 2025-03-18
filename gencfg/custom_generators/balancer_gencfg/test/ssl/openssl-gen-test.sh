#! /usr/bin/env bash

if [ $# != 1 ]; then
    echo "Usage: `basename $0` <output dir>";
    exit 1;
fi

OUTPUT_DIR=$1;

mkdir -p ${OUTPUT_DIR} || exit 1;

openssl req -batch -nodes -config `dirname $0`/openssl-gen-test.cnf -new -out ${OUTPUT_DIR}/request.cert -keyout ${OUTPUT_DIR}/private.cert
openssl x509 -req -days 365 -in ${OUTPUT_DIR}/request.cert -signkey ${OUTPUT_DIR}/private.cert -out ${OUTPUT_DIR}/public.cert
