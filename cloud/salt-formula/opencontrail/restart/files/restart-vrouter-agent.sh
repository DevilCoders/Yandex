#!/bin/bash

echo "$(date) contrail-vrouter-agent is going to RESTART at $(hostname)..."
set -x

systemctl restart contrail-vrouter-agent

set +x
echo "$(date) completed."
