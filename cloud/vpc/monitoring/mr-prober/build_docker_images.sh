#!/usr/bin/env bash

set -e

print_usage() {
  echo "USAGE: ./build_docker_images.sh [-p] [-t testing] [-l] [all|api|agent|creator|synchronize|routing_bingo]" >&2
  echo "  -p — push docker images to the container registry (disabled by default)" >&2
  echo "  -i — ignore protobufs generating"
  echo "  -t testing — add tag 'testing' to all docker images ('latest' by default)" >&2
  echo "  -l — add tag 'latest' to all docker images (if -t is used)" >&2
  echo "  You can specify which image you want to build. By default it builds all Mr. Prober images." >&2
  echo "  IMPORTANT! This script requires Bash version 4 or later. On macOS use version from \"brew install bash\"" >&2
}

cd "$(dirname "$0")"

PYTHON_VERSION=3.10.1

IMAGES_TAG=latest
CONTAINER_REGISTRY=cr.yandex/crpni6s1s1aujltb5vv7

if ! declare -A IMAGES=(
  ["api"]="api/Dockerfile"
  ["agent"]="agent/Dockerfile"
  ["creator"]="creator/Dockerfile"
  ["synchronize"]="tools/synchronize/Dockerfile"
  ["routing_bingo"]="tools/routing_bingo/Dockerfile"
) 2> /dev/null; then
    echo "Your bash version $BASH_VERSION doesn't support associative arrays. Upgrade it to bash 4.0+. " >&2
    echo "If you're running macOS, try \"brew install bash\" and restart your shell." >&2
    exit 1
fi


# 1. Parse options
PUSH=""
IGNORE_PROTOBUFS_GENERATING=""
IMAGES_TAG_LATEST=""
while getopts "hiplt:" opt
do
  case $opt in
    p)
      PUSH=1
      ;;
    i)
      IGNORE_PROTOBUFS_GENERATING=1
      ;;
    l)
      IMAGES_TAG_LATEST=1
      ;;
    t)
      IMAGES_TAG="$OPTARG"
      ;;
    h)
      print_usage
      exit 0
      ;;
    *)
      print_usage
      exit 1
      ;;
  esac
done

IMAGES_SPECIFICATION=${*:$OPTIND:1}
if [[ -z $IMAGES_SPECIFICATION ]]
then
  IMAGES_SPECIFICATION="all"
fi

# 2. Generate code by Yandex Cloud's protospecs.
# Run ./build_proto_specs.sh in the python container with mounted arcadia.
# It will generate (or update) code in googleapis/ and yandex/.
if [[ ! $IGNORE_PROTOBUFS_GENERATING ]]
then
  echo "[!] Generating protobufs"
  docker run --rm --platform linux/amd64 -v "$PWD/../../../:/arcadia/cloud" python:$PYTHON_VERSION /arcadia/cloud/vpc/monitoring/mr-prober/build_proto_specs.sh
fi

# 3. Build (and push) docker images for Mr. Prober
for image in "${!IMAGES[@]}"
do
  dockerfile=${IMAGES[$image]}

  if [[ $IMAGES_SPECIFICATION == "$image" || $IMAGES_SPECIFICATION == "all" ]]
  then
    echo -e "\n[!] Building $image image from $dockerfile as $CONTAINER_REGISTRY/$image:$IMAGES_TAG"
    docker build --platform linux/amd64 --build-arg PYTHON_VERSION=$PYTHON_VERSION . -t $CONTAINER_REGISTRY/"$image":"$IMAGES_TAG" -f "$dockerfile"

    docker_image_id=$(docker images $CONTAINER_REGISTRY/"$image":"$IMAGES_TAG" --format '{{.ID}}')
    if [[ $IMAGES_TAG_LATEST ]]
    then
      echo -e "[!] Mark $CONTAINER_REGISTRY/$image:$IMAGES_TAG as $CONTAINER_REGISTRY/$image:latest also"
      docker tag $CONTAINER_REGISTRY/"$image":"$IMAGES_TAG" $CONTAINER_REGISTRY/"$image":latest
    fi

    if [[ $PUSH ]]
    then
      echo -e "\n[!] Pushing $image image to $CONTAINER_REGISTRY/$image:$IMAGES_TAG"
      docker push $CONTAINER_REGISTRY/"$image":"$IMAGES_TAG"
      if [[ $IMAGES_TAG_LATEST ]]
      then
        docker push $CONTAINER_REGISTRY/"$image":latest
      fi
    fi
  fi
done
