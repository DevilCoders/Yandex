#!/bin/bash
set -x
BASE_DIR="${pwd}"
CLIENTS="ozon decathlon edadeal"

log() {
    TIMESTAMP=$(date "+%Y-%m-%d %H:%M:%S")
    msg="$TIMESTAMP $1"
    echo $msg
}

update_client_alerts() {
    client=$1
    mv ${BASE_DIR}/${client}_config.yaml.next ${BASE_DIR}/${client}_config.yaml
    yc-solomon-cli -c ${BASE_DIR}/../../config.yaml -i main -e prod -m apply update -o slo-$client-alerts
}

log "Updating yc-solomon-cli configs"
python3 generate_alert.cfg.py

for client in $CLIENTS; do
   log "Check $client"
   diff ${BASE_DIR}/${client}_config.yaml.next ${BASE_DIR}/${client}_config.yaml && log "$client without diff" || update_client_alerts $client
done
