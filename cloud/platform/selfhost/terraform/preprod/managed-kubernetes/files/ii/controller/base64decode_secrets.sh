#!/bin/bash

set -ex

base64 -d < /etc/mk8s-controller/kubeconfig.yaml.enc.b64 > /etc/mk8s-controller/kubeconfig.yaml.enc
base64 -d < /etc/mk8s-controller/yc-sa-key.json.enc.b64 > /etc/mk8s-controller/yc-sa-key.json.enc
base64 -d < /etc/mk8s-controller/s3_viewer_service_account_key.json.enc.b64 > /etc/mk8s-controller/s3_viewer_service_account_key.json.enc
base64 -d < /etc/mk8s-controller/s3_editor_service_account_key.json.enc.b64 > /etc/mk8s-controller/s3_editor_service_account_key.json.enc
base64 -d < /etc/mk8s-controller/cr-yandex-sa-key.json.enc.b64 > /etc/mk8s-controller/cr-yandex-sa-key.json.enc
base64 -d < /etc/mk8s-controller/deks/initial.enc.b64 > /etc/mk8s-controller/deks/initial.enc
base64 -d < /var/lib/kubelet/config.json.enc.b64 > /var/lib/kubelet/config.json.enc
base64 -d < /etc/metricsagent/oauth_token.enc.b64 > /etc/metricsagent/oauth_token.enc
