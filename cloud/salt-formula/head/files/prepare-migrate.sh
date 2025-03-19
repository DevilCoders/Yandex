#!/usr/bin/env bash

set -e

function start_compute {
    systemctl start yc-compute-api
    systemctl start yc-compute-worker
}
# always start on finish
trap start_compute EXIT

systemctl stop yc-compute-api
systemctl stop yc-compute-worker

yc-compute-admin db online-prepare
