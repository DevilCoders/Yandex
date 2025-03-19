#!/bin/bash

pmm-admin config --server ${PMM_SERVER}:${PMM_SERVER_PORT} --client-address $(hostname) --bind-address $(host $(hostname) | awk '{ print $NF }') --force
sed -i '/^bind_address:/d' /usr/local/percona/pmm-client/pmm.yml
pmm-admin add linux:metrics --service-port 43120
pmm-admin add mysql:metrics --service-port 43122
pmm-admin add mysql:queries
echo
pmm-admin list
