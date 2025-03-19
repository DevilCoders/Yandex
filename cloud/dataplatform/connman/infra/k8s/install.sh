#!/bin/bash

set -e

source setup_kubectl.sh

helm repo --kube-context "$K8S_CONTEXT" add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo --kube-context "$K8S_CONTEXT" update

echo "installing ingress nginx..."
UPGRADE_CMD="upgrade --install --kube-context $K8S_CONTEXT ingress-nginx ingress-nginx/ingress-nginx -f ingress-nginx-helm.yaml"
# shellcheck disable=SC2086
helm diff $UPGRADE_CMD --detailed-exitcode --output simple || helm $UPGRADE_CMD

echo "installing cluster ip..."
kubectl apply -f cluster-ip.yaml --context "$K8S_CONTEXT"

echo "installing tls secret..."
TLS_CERT_FILE="/tmp/connman_cert_$PROFILE.pem"
TLS_KEY_FILE="/tmp/connman_key_$PROFILE.pem"
yc cm certificate content --profile "$YC_PROFILE" --id "$CERTIFICATE_ID" --chain "$TLS_CERT_FILE" --key "$TLS_KEY_FILE" >/dev/null
kubectl create secret tls connman-tls --cert "$TLS_CERT_FILE" --key "$TLS_KEY_FILE" --dry-run=client -o yaml --save-config --context "$K8S_CONTEXT" |
  kubectl apply -f - --context "$K8S_CONTEXT"
rm "$TLS_CERT_FILE"
rm "$TLS_KEY_FILE"

echo "installing ingress grpc..."
INGRESS_GRPC_FILE="ingress-grpc_$PROFILE.yaml"
envsubst <ingress-grpc.yaml >"$INGRESS_GRPC_FILE"
kubectl apply -f "$INGRESS_GRPC_FILE" --context "$K8S_CONTEXT"
rm "$INGRESS_GRPC_FILE"

echo "installing ingress http..."
INGRESS_HTTP_FILE="ingress-http_$PROFILE.yaml"
envsubst <ingress-http.yaml >"$INGRESS_HTTP_FILE"
kubectl apply -f "$INGRESS_HTTP_FILE" --context "$K8S_CONTEXT"
rm "$INGRESS_HTTP_FILE"
