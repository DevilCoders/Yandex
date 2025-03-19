#!/bin/bash

# This script should be run after setting up a new host:
#  * installs osquery and sets the right osquery tag

set -ex

OSQUERY_VANILLA_VERSION=4.8.0.1
OSQUERY_CONFIG_VERSION=1.1.1.59
OSQUERY_TAG=ycloud-svc-kms

# Install and configure osquery
pssh run "sudo apt purge auditd; sudo systemctl mask systemd-journald-audit.socket; sudo systemctl stop systemd-journald-audit.socket" $KMS_HOST_NAME
pssh run "echo -n $OSQUERY_TAG | sudo tee /etc/osquery.tag" $KMS_HOST_NAME
pssh run "sudo apt update && sudo apt install --reinstall osquery-vanilla=$OSQUERY_VANILLA_VERSION osquery-yandex-generic-config=$OSQUERY_CONFIG_VERSION" $KMS_HOST_NAME
