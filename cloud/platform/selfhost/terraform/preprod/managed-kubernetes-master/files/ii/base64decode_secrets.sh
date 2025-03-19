#!/bin/bash

set -ex

base64 -d < /etc/kubernetes/pki/apiserver.crt.enc.b64 > /etc/kubernetes/pki/apiserver.crt.enc
base64 -d < /etc/kubernetes/pki/apiserver.key.enc.b64 > /etc/kubernetes/pki/apiserver.key.enc
base64 -d < /etc/kubernetes/pki/ca.crt.enc.b64 > /etc/kubernetes/pki/ca.crt.enc
base64 -d < /etc/kubernetes/pki/ca.key.enc.b64 > /etc/kubernetes/pki/ca.key.enc
base64 -d < /etc/kubernetes/pki/etcd-apiserver-client.crt.enc.b64 > /etc/kubernetes/pki/etcd-apiserver-client.crt.enc
base64 -d < /etc/kubernetes/pki/etcd-apiserver-client.key.enc.b64 > /etc/kubernetes/pki/etcd-apiserver-client.key.enc
base64 -d < /etc/kubernetes/pki/etcd-ca.crt.enc.b64 > /etc/kubernetes/pki/etcd-ca.crt.enc
base64 -d < /etc/kubernetes/pki/etcd-ca.key.enc.b64 > /etc/kubernetes/pki/etcd-ca.key.enc
base64 -d < /etc/kubernetes/pki/front-proxy-ca.crt.enc.b64 > /etc/kubernetes/pki/front-proxy-ca.crt.enc
base64 -d < /etc/kubernetes/pki/front-proxy-ca.key.enc.b64 > /etc/kubernetes/pki/front-proxy-ca.key.enc
base64 -d < /etc/kubernetes/pki/front-proxy-client.crt.enc.b64 > /etc/kubernetes/pki/front-proxy-client.crt.enc
base64 -d < /etc/kubernetes/pki/front-proxy-client.key.enc.b64 > /etc/kubernetes/pki/front-proxy-client.key.enc
base64 -d < /etc/kubernetes/pki/kubelet-apiserver-client.crt.enc.b64 > /etc/kubernetes/pki/kubelet-apiserver-client.crt.enc
base64 -d < /etc/kubernetes/pki/kubelet-apiserver-client.key.enc.b64 > /etc/kubernetes/pki/kubelet-apiserver-client.key.enc
base64 -d < /etc/kubernetes/pki/sa.key.enc.b64 > /etc/kubernetes/pki/sa.key.enc
base64 -d < /etc/kubernetes/pki/sa.pub.enc.b64 > /etc/kubernetes/pki/sa.pub.enc
base64 -d < /etc/kubernetes/admin-kubeconfig.conf.enc.b64 > /etc/kubernetes/admin-kubeconfig.conf.enc
base64 -d < /etc/kubernetes/kube-controller-manager-kubeconfig.conf.enc.b64 > /etc/kubernetes/kube-controller-manager-kubeconfig.conf.enc
base64 -d < /etc/kubernetes/kube-scheduler-kubeconfig.conf.enc.b64 > /etc/kubernetes/kube-scheduler-kubeconfig.conf.enc
base64 -d < /var/lib/kubelet/config.json.enc.b64 > /var/lib/kubelet/config.json.enc
base64 -d < /etc/metricsagent/oauth_token.enc.b64 > /etc/metricsagent/oauth_token.enc
base64 -d < /etc/metricsagent/kubeconfig.yaml.enc.b64 > /etc/metricsagent/kubeconfig.yaml.enc
