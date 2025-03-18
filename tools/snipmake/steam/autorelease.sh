#!/bin/sh

set -o errexit
set -o nounset

PROJECT_ROOT=`pwd .`
EMAIL=teamcity@yandex-team.ru
DEBUILD_CMD="dpkg-buildpackage -rfakeroot -sa -e${EMAIL} -I"
DEBCLEAN_CMD="debclean"
REPO_NAME="yandex-precise"
DEBRELEASE_CMD="debrelease --to ${REPO_NAME} --nomail"

if [ -n "$1"  ]; then
    PROJECT_DIR="${PROJECT_ROOT}/$1/"
else
    echo 'Not enough command line arguments - project name not specified'
    exit 1
fi

if [ ! -e "${PROJECT_DIR}/debian/changelog"  ]; then
    echo 'debian/changelog not found'
    exit 2
fi

#Build phase
echo "Build debian package..."
cd $PROJECT_DIR && $DEBUILD_CMD || exit 3

echo "Debian clean..."
cd $PROJECT_DIR && $DEBCLEAN_CMD || exit 4

#Deploy phase
DEB_SOURCE=$(dpkg-parsechangelog | sed -n 's/^Source: //p')
DEB_VERSION=$(dpkg-parsechangelog | sed -n 's/^Version: //p')
DEB_ARCH=$(dpkg-architecture | grep DEB_BUILD_ARCH= | sed -e 's/.*=//')
CHANGES_FILE="${DEB_SOURCE}\_${DEB_VERSION}\_${DEB_ARCH}.changes"

PACKAGES_IN_REPO=$(ssh dist.yandex.ru "cd /repo/${REPO_NAME} && find . -name '${CHANGES_FILE}' | wc -l")

PACKAGE_ID=${DEB_SOURCE}-${DEB_VERSION}

echo "Deploy package..."

if [ "${PACKAGES_IN_REPO}" = "0"  ]; then
    echo "Release package to ${REPO_NAME}..."
    cd $PROJECT_DIR && $DEBRELEASE_CMD || exit 5
else
    echo "Package ${PACKAGE_ID} is already in ${REPO_NAME} repo. Conductor ticket will not be created."
fi
