#!/bin/bash
set -e

source "$(dirname $0)/../os-env.sh"

VERSION=""
if [ $# -gt 0 ]; then
    VERSION=$1
    echo "VERSION=$VERSION"
else
    echo "First parameter should be the new VERSION"
    exit 1
fi

PLUGIN_NAME=kubectl-saltformula
CHECKSUMS_FOLDER=.cache
OUTPUT_FOLDER=$(arc root)/cloud/bootstrap/k8s/krew-index/plugins

DARWIN_AMD64=$(grep $PLUGIN_NAME $CHECKSUMS_FOLDER/*checksums.txt  | grep "darwin_amd64" | awk '{print $1}')
DARWIN_ARM64=$(grep $PLUGIN_NAME $CHECKSUMS_FOLDER/*checksums.txt  | grep "darwin_arm64" | awk '{print $1}')
WINDOWS_AMD64=$(grep $PLUGIN_NAME $CHECKSUMS_FOLDER/*checksums.txt  | grep "windows_amd64" | awk '{print $1}')
LINUX_AMD64=$(grep $PLUGIN_NAME $CHECKSUMS_FOLDER/*checksums.txt  | grep "linux_amd64" | awk '{print $1}')

echo "DARWIN_AMD64=$DARWIN_AMD64"
echo "WINDOWS_AMD64=$WINDOWS_AMD64"
echo "LINUX_AMD64=$LINUX_AMD64"

cp hack/release/saltformula-tmpl.yaml $OUTPUT_FOLDER/saltformula.yaml

$SED "s/PLACEHOLDER_VERSION/$VERSION/g" $OUTPUT_FOLDER/saltformula.yaml
$SED "s/PLACEHOLDER_SHA_DARWIN_AMD64/$DARWIN_AMD64/g" $OUTPUT_FOLDER/saltformula.yaml
$SED "s/PLACEHOLDER_SHA_DARWIN_ARM64/$DARWIN_ARM64/g" $OUTPUT_FOLDER/saltformula.yaml
$SED "s/PLACEHOLDER_SHA_LINUX/$LINUX_AMD64/g" $OUTPUT_FOLDER/saltformula.yaml
$SED "s/PLACEHOLDER_SHA_WINDOWS/$WINDOWS_AMD64/g" $OUTPUT_FOLDER/saltformula.yaml
