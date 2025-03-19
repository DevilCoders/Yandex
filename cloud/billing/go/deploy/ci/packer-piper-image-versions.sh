#!/bin/bash
set -xe

# Update piper docker image versions in packer variables file
# Sandbox resources needed!
# Script needs jq tool from sandbox resources and setted env variable JQ_BIN to path to this resource.

JQ=${JQ_BIN:-jq}

PIPER_VERSION="rel-$1"
TOOLS_VERSION=`$JQ -r .meta.version $ARCADIA_PATH/cloud/billing/go/deploy/versions/tools.json`

$JQ -n --arg v "$PIPER_VERSION" '{"meta": {"version": $v}}' > $ARCADIA_PATH/cloud/billing/go/deploy/versions/piper.json

cat > $ARCADIA_PATH/cloud/billing/go/deploy/packer/piper/image_versions.pkrvars.hcl <<EOF
# piper images
config_image_version = "$PIPER_VERSION"
piper_image_version  = "$PIPER_VERSION"

# hepler images
tools_image_version  = "$TOOLS_VERSION"
EOF

echo $PIPER_VERSION > $RESULT_RESOURCES_PATH/piper_version
echo $TOOLS_VERSION > $RESULT_RESOURCES_PATH/tools_version
