#!/bin/bash

set -ex

mkdir -p /root/.kube

ln -f -s /etc/mk8s-controller/kubeconfig.yaml /root/.kube/config
