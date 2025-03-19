#!/bin/bash

YC_PROFILE="connman-$PROFILE"
K8S_CONTEXT=$YC_PROFILE
yc managed-kubernetes cluster get-credentials --id "$K8S_ID" --external --context-name "$K8S_CONTEXT" --force --profile "$YC_PROFILE"
