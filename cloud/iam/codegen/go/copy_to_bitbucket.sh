#!/bin/sh
set -ex
cd $(dirname $0)
ya make -r --add-result=.go
cp permissions/permissions.go "${GOPATH}/src/bb.yandex-team.ru/cloud/cloud-go/iam/codegen/permissions/permissions.go"
cp quotas/quotas.go "${GOPATH}/src/bb.yandex-team.ru/cloud/cloud-go/iam/codegen/quotas/quotas.go"
cp resources/resources.go "${GOPATH}/src/bb.yandex-team.ru/cloud/cloud-go/iam/codegen/resources/resources.go"

