#!/bin/sh -e

./build.sh
docker push registry.yandex.net/cloud/platform/java11:latest
docker push cr.yandex/crp6ro8l0u0o3qgmvv3r/java11:latest
