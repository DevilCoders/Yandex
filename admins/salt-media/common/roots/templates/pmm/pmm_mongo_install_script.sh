#!/bin/bash

MONGO_GRANTS_FILE="/var/cache/mongo-grants/mongo_grants_config"
if [ -f "$MONGO_GRANTS_FILE" ]
then
    MONGO_PASS=$(awk -F'\\s+' -v section="[users]" -v key="monitoring" '$0 == section { f=1; next } /^\[.*\]$/ { f=0; next } f && $1 == key { print $2; exit }' $MONGO_GRANTS_FILE)
fi

pmm-admin config --server ${PMM_SERVER}:${PMM_SERVER_PORT} --client-address $(hostname) --bind-address $(host $(hostname) | awk '{ print $NF }')
sed -i '/^bind_address:/d' /usr/local/percona/pmm-client/pmm.yml
pmm-admin add linux:metrics --service-port 43120
if [ -n "$MONGO_PASS" ]
then
    pmm-admin add mongodb:metrics --uri "monitoring:${MONGO_PASS}@127.0.0.1:${PMM_MONGO_PORT}" --cluster "${PMM_MONGO_CLUSTER}"
    pmm-admin add mongodb:queries --uri "monitoring:${MONGO_PASS}@127.0.0.1:${PMM_MONGO_PORT}"
else
    pmm-admin add mongodb:metrics --uri "127.0.0.1:${PMM_MONGO_PORT}" --cluster "${PMM_MONGO_CLUSTER}"
    pmm-admin add mongodb:queries --uri "127.0.0.1:${PMM_MONGO_PORT}"
fi

echo
pmm-admin list
