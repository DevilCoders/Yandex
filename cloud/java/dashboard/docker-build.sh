./compile.sh
ya package package.json --docker --docker-registry=registry.yandex.net --docker-repository=cloud/platform
docker tag registry.yandex.net/cloud/platform/dashboard:latest cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest
