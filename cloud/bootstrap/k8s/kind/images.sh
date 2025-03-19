#!/bin/bash
KIND_CLUSTER=$1
images[0]="cr.yandex/yc-bootstrap/argocd/redis:6.2.1-alpine"
images[1]="cr.yandex/yc-bootstrap/argocd/argoproj/argocd:v2.0.0"
images[2]="cr.yandex/yc-bootstrap/argocd/dexidp/dex:v2.27.0"
images[3]="cr.yandex/yc-bootstrap/argocd/argoproj/argocli:v3.0.2"
images[4]="cr.yandex/yc-bootstrap/argocd/postgres:12-alpine"
images[5]="cr.yandex/yc-bootstrap/argocd/minio/minio:RELEASE.2019-12-17T23-16-33Z"
images[6]="cr.yandex/yc-bootstrap/argocd/argoproj/workflow-controller:v3.0.2"
images[7]="cr.yandex/yc-bootstrap/argocd/argoproj/argoexec:v3.0.2"
images[8]="cr.yandex/yc-bootstrap/calico/pod2daemon-flexvol:v3.14.2"
images[9]="cr.yandex/yc-bootstrap/calico/node:v3.14.2"
images[10]="cr.yandex/yc-bootstrap/calico/kube-controllers:v3.14.2"
images[11]="cr.yandex/yc-bootstrap/calico/cni:v3.14.2"
images[12]="cr.yandex/yc-bootstrap/argocd/busybox:latest"
images[13]="cr.yandex/yc-bootstrap/argocd/ip-masq-agent-amd64:v2.6.0"
images[14]="cr.yandex/yc-bootstrap/argocd/alpine:3.7"
images[15]="cr.yandex/yc-bootstrap/argocd/eventbus-controller:v1.3.1"
images[16]="cr.yandex/yc-bootstrap/argocd/eventsource-controller:v1.3.1"
images[17]="cr.yandex/yc-bootstrap/argocd/sensor-controller:v1.3.1"

for image in "${images[@]}";do
    docker pull "$image"
    kind load docker-image --name "${KIND_CLUSTER}" ${image}
done
