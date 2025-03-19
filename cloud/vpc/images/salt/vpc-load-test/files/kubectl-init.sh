#!/usr/bin/env bash

echo "" | yc init || true
yc managed-kubernetes cluster get-credentials load-test-cluster --internal --force
yc container registry configure-docker