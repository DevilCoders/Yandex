#!/bin/bash
usage="$(basename "$0") [-h] [-t TAG] -- move mkt docker images from yandex registry to cloud registry

where:
    -h  show this help text and exit
    -t TAG is the docker tag to clone"


while getopts ':ht:' option; do
  case "$option" in
    h) echo "$usage"
       exit
       ;;
    t) TAG="$OPTARG"
       echo "cloning worker container $TAG"
       docker pull registry.yandex.net/cloud/marketplace-worker:"$TAG"
       docker tag registry.yandex.net/cloud/marketplace-worker:"$TAG" cr.yandex/crpo2khtm9ocefi307m8/marketplace-v1-worker:"$TAG"
       docker push cr.yandex/crpo2khtm9ocefi307m8/marketplace-v1-worker:"$TAG"
       echo "cloning api container $TAG"
       docker pull registry.yandex.net/cloud/marketplace:"$TAG"
       docker tag registry.yandex.net/cloud/marketplace:"$TAG" cr.yandex/crpo2khtm9ocefi307m8/marketplace-v1:"$TAG"
       docker push cr.yandex/crpo2khtm9ocefi307m8/marketplace-v1:"$TAG" 
       ;;
    :) printf "missing argument for -%s\n" "$OPTARG" >&2
       echo "$usage" >&2
       exit 1
       ;;
    \?) printf "illegal option: -%s\n" "$OPTARG" >&2
       echo "$usage" >&2
       exit 1
       ;;
  esac
done


