#!/usr/bin/env bash
THIS_SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
[ "$IAM_METADATA_ENV_VARS_AVAILABLE" != "yes" ] && echo "please init settings and variables by running \`source ./env.sh\`" && exit 1

set -e
set -o pipefail
TICKET=${TICKET:?"TICKET env variable is required"}
DATE=$(date "+%Y-%m-%d")
COMMIT=$(git -C "$CLOUD_GO_PATH" rev-parse HEAD | head -c 8)
S3_PATH="$S3_BUCKET/metadata/releases/$DATE/$COMMIT"
S3_TARGET="s3://$S3_PATH"
LOCAL_DIR="./current"
mkdir -p $LOCAL_DIR
YCS3_CMD="aws --profile $S3_AWS_PROFILE --endpoint-url $S3_ENDPOINT s3"

echo " == Preparing compiled from local yaml files in $LOCAL_DIR == "
for profile in $STANDS; do
  out="${LOCAL_DIR}/${profile}_dump.yaml"
  test -f $out && echo "File $out exists, please remove (use ./cleanup.sh)" && exit 1
  echo -ne " === $profile ==="
  cmd="ycp --profile $profile iam inner metadata dump --allow-warnings"
  $cmd >$out && echo "OK"
done

echo " == Exporting actual state snapshots from environments == "
for profile in $STANDS; do
  out="${LOCAL_DIR}/${profile}_export.yaml"
  test -f $out && echo "File $out exists, please remove (use ./cleanup.sh)" && exit 1
  echo -e " === $profile ==="
  cmd="ycp --profile $profile iam inner metadata export"
  $cmd >$out && echo "OK"
done

echo " == Uploading release artifacts to $S3_TARGET =="
comment='Uploaded release artifacts:
'
for profile in $STANDS; do
  $YCS3_CMD cp ${LOCAL_DIR}/${profile}_export.yaml $S3_TARGET/ --content-type text/vnd.yaml
  $YCS3_CMD cp ${LOCAL_DIR}/${profile}_dump.yaml $S3_TARGET/ --content-type text/vnd.yaml
  comment="$comment
$S3_ENDPOINT/$S3_PATH/${profile}_export.yaml
$S3_ENDPOINT/$S3_PATH/${profile}_dump.yaml
"
done
comment="$comment
"

${THIS_SCRIPT_DIR}/../add_startrek_comment.sh $TICKET "$comment"
