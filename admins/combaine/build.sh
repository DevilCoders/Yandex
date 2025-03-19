#! /bin/bash
set -eu

DO_PUSH=0
for i in "$@"; do
    case $i in
    -v=*) VERSION="${i#*=}" ;;
    --no-cache) NO_CACHE="${NO_CACHE:-} --no-cache" ;;
    --pull) NO_CACHE="${NO_CACHE:-} --pull" ;;
    --push) DO_PUSH=1 ;;
    *)
        echo "$0 -v=<version> [--push]"
        echo " -v=           image version"
        echo " --no-cache    force cache rebuild"
        echo " --pull        force docker pull"
        echo " --push        docker push"
        exit 1
        ;;
    esac
done

set -x

echo "VERSION=${VERSION:=$(date +%FT%T|tr ':' '.')}"
docker build ${NO_CACHE:-} -t combainer-internal:$VERSION .

docker tag combainer-internal:$VERSION registry.yandex.net/media/combainer:$VERSION
if ((DO_PUSH && $DO_PUSH)); then
    docker push registry.yandex.net/media/combainer:$VERSION
fi
