#!/bin/bash

set -ex

base64 -d < /etc/mk8s-billcollector/kubeconfig.yaml.enc.b64 > /etc/mk8s-billcollector/kubeconfig.yaml.enc
base64 -d < /var/lib/kubelet/config.json.enc.b64 > /var/lib/kubelet/config.json.enc
base64 -d < /etc/metricsagent/oauth_token.enc.b64 > /etc/metricsagent/oauth_token.enc
