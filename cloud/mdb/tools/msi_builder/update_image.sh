#!/bin/bash
docker build --tag=registry.yandex.net/dbaas/msi-builder .
docker push registry.yandex.net/dbaas/msi-builder
