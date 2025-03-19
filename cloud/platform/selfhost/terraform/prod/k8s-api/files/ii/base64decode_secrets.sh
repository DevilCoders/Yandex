#!/bin/bash

set -ex

base64 -d < /etc/k8sapi/kubeconfig.yaml.enc.b64 > /etc/k8sapi/kubeconfig.yaml.enc
base64 -d < /etc/k8sapi/yc-sa-key.json.enc.b64 > /etc/k8sapi/yc-sa-key.json.enc
base64 -d < /etc/k8sapi/ssl/certs/server.crt.enc.b64 > /etc/k8sapi/ssl/certs/server.crt.enc
base64 -d < /etc/k8sapi/ssl/private/server.key.enc.b64 > /etc/k8sapi/ssl/private/server.key.enc
base64 -d < /var/lib/kubelet/config.json.enc.b64 > /var/lib/kubelet/config.json.enc
base64 -d < /etc/metricsagent/oauth_token.enc.b64 > /etc/metricsagent/oauth_token.enc
