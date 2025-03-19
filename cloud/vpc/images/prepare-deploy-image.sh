#!/usr/bin/env bash

set -eu

if [ $# -le 0 ] || [ -z "$1" ] ; then
    echo "Usage: $0 DIR"
    exit 2
fi

DIR=$(echo $1 | sed 's#/$##g')
PARENT_DIR=$(dirname "$DIR")
BASE_DIR=$(dirname $(realpath $0))
DEPLOYHOST="deploy.cloud.yandex.net"
PROFILETMP=$(mktemp)
trap "rm $PROFILETMP" EXIT

# NOTE: this uses cloud-tools-prod-nets subnet for exporting,
# and requires use_ipv6 in post-processor
cat >$PROFILETMP <<EOF
export FOLDER_ID="b1gn8lr1h94mi3lfq2qa"
export SUBNET_ID="e9bigio0vhk246euavkb"
export ZONE_ID="ru-central1-a"
export YC_TOKEN=$(yc --profile=prod iam create-token)
export COMMIT_AUTHOR="$(whoami)"
export SERVICE_ACCOUNT_ID="aje89rf6e10u9d9ucun1"

export EXPORT_SUBNET_ID="e9bigio0vhk246euavkb"
export EXPORT_SERVICE_ACCOUNT_ID="aje0jeuv14jjbeqe4c4e"
export EXPORT_STORAGE_PATH="s3://yc-vpc-packer-export/"
EOF

echo "Copying salt/ ..."
pssh scp $BASE_DIR/salt $DEPLOYHOST:

echo "Creating $PARENT_DIR/ and cleaning $DIR/ ..."
pssh run "mkdir -p $PARENT_DIR; rm -rfv $DIR || true" $DEPLOYHOST

echo "Copying $DIR/ ..."
pssh scp -R $BASE_DIR/$DIR $DEPLOYHOST:$DIR

echo "Copying profile"
pssh scp $PROFILETMP $DEPLOYHOST:$DIR/profile
pssh run "chmod 0600 $DIR/profile" $DEPLOYHOST

# NOTE: requires latest packer binary to be downloaded into home directory
echo $DEPLOYHOST:$DIR/ is prepared
echo
echo Now do the following:
echo "   pssh -A $DEPLOYHOST"
echo "   cd $DIR"
echo "   . profile && rm profile"
echo '   ~/packer build ./packer.json'
echo '(packer binary has to be downloaded to the home directory)'
