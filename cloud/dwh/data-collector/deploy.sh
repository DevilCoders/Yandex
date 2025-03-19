app="registry.yandex.net/yc-dwh/data-collector"
VER=1.1.1
TAG=${app}:${VER}
echo tag: ${TAG}
docker build . -t ${TAG}
docker push ${TAG}
