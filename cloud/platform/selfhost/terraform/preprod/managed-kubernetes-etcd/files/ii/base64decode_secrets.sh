#!/bin/bash

set -ex

base64 -d < /etc/kubernetes/pki/etcd/ca.crt.enc.b64 > /etc/kubernetes/pki/etcd/ca.crt.enc
base64 -d < /etc/kubernetes/pki/etcd/healthcheck-client.crt.enc.b64 > /etc/kubernetes/pki/etcd/healthcheck-client.crt.enc
base64 -d < /etc/kubernetes/pki/etcd/healthcheck-client.key.enc.b64 > /etc/kubernetes/pki/etcd/healthcheck-client.key.enc
base64 -d < /etc/kubernetes/pki/etcd/server.crt.enc.b64 > /etc/kubernetes/pki/etcd/server.crt.enc
base64 -d < /etc/kubernetes/pki/etcd/server.key.enc.b64 > /etc/kubernetes/pki/etcd/server.key.enc
base64 -d < /etc/kubernetes/pki/etcd/peer.crt.enc.b64 > /etc/kubernetes/pki/etcd/peer.crt.enc
base64 -d < /etc/kubernetes/pki/etcd/peer.key.enc.b64 > /etc/kubernetes/pki/etcd/peer.key.enc
base64 -d < /var/lib/kubelet/config.json.enc.b64 > /var/lib/kubelet/config.json.enc
base64 -d < /etc/metricsagent/oauth_token.enc.b64 > /etc/metricsagent/oauth_token.enc
