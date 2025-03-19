#!/bin/bash

set -ex

export VERSION=1.4.19
docker build --platform linux/amd64 -t cr.yandex/crpkne7bucbk4155b9uf/dutybot:$VERSION .
docker push cr.yandex/crpkne7bucbk4155b9uf/dutybot:$VERSION
