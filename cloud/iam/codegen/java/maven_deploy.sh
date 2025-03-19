#!/usr/bin/env bash

set -eu
ARCADIA_ROOT="${PWD}"

while [[ ! -e "${ARCADIA_ROOT}/.arcadia.root" ]]; do
    if [[ "${ARCADIA_ROOT}" == "/" ]]; then
        echo "$0: can't find root, must be run within Arcadia working copy" >&2
        exit 1
    fi
    ARCADIA_ROOT="$(dirname "${ARCADIA_ROOT}")"
done

echo "Building from arcadia ${ARCADIA_ROOT}"

cd ${ARCADIA_ROOT}

if [ $(./ya dump vcs-info | jq -r '.VCS') == "svn" ]; then
    svn up .
fi

ARC_VERSION=$(./ya dump vcs-info | jq '.ARCADIA_SOURCE_REVISION')
if [ ${ARC_VERSION} == -1 ]; then
  echo "Invalid svn revision. Please switch to trunk."
  exit 1
fi

echo "Build for version ${ARC_VERSION}"

MODULES="cloud/iam/codegen/java/lib"
JDK_VERSION=11

./ya make -DJDK_VERSION=$JDK_VERSION --checkout -r -j8 $MODULES
./ya make -DJDK_VERSION=$JDK_VERSION -r -j8 --maven-export --deploy --repository-id yandex-releases --repository-url http://artifactory.yandex.net/artifactory/yandex_common_releases/ -s --version=$ARC_VERSION $MODULES
