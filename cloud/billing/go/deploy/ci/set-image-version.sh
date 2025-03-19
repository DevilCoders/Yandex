#!/bin/bash

set -e

# Script generates new packer image version
# Args:
#   $1 - packer folder name
#   $2 - release number

VERSION=$(echo $2|sed -e 's/[.]/-/')

IMAGE="billing-$1-$VERSION-`date +%Y-%m-%d-%H%M%S`"

cd $ARCADIA_PATH/cloud/billing/go/deploy/packer/$1
echo output_image_name = \"$IMAGE\" > output_image.pkrvars.hcl

cd $ARCADIA_PATH/cloud/billing/go/deploy/terraform/

TERRAGRUNT_FILE=${1}_version.hcl
cat > $TERRAGRUNT_FILE << EOF
locals {
    image_name = "$IMAGE"
}
EOF

echo $IMAGE > $RESULT_RESOURCES_PATH/image
