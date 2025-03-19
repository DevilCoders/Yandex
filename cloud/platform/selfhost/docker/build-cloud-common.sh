if [[ -z "$1" ]]; then
    echo "required name!"
    exit 1
fi
NAME="$1"
OLD_DIR=$(pwd)
NEW_DIR=./${NAME}
VERSION=$(date +%Y-%m-%dT%H-%M)
cd ${NEW_DIR}
docker build ./ --no-cache -t ${NAME}:${VERSION}
OLD_TAG="registry.yandex.net/cloud/platform/${NAME}:${VERSION}"
docker tag ${NAME}:${VERSION} ${OLD_TAG}
echo ${OLD_TAG}
NEW_TAG="cr.yandex/yc-internal/${NAME}:${VERSION}"
docker tag ${NAME}:${VERSION} ${NEW_TAG}
echo ${NEW_TAG}
cd ${OLD_DIR}
