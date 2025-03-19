#!/bin/sh -e

./docker-build.sh
docker push registry.yandex.net/cloud/platform/dashboard:latest
docker push cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest
