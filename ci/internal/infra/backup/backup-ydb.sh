# Example: https://a.yandex-team.ru/arc_vcs/ci/internal/infra/backup/a.yaml?rev=r9189389

set -ex
GEESEFS=$1
JQ=$2
YA_BIN="ya ydb -e ydb-ru.yandex.net:2135 -d $YDB_DIR"

S3_BUCKET=warehouse-ci
S3_PATH=$YDB_NAME/backup

echo "Backup for $YDB_DIR to $S3_BUCKET/$S3_PATH"

function get() {
  ID=$1
  FUNC=$2
  echo $($YA_BIN operation get $ID --format proto-json-base64 | $JQ -r $FUNC)
}

function get_ready() {
  get $1 ".ready"
}

function get_status() {
  get $1 ".status"
}

echo "Closing old YDB export operations"
for YDB_ID in $($YA_BIN operation list export/s3 --format proto-json-base64 | $JQ -r ".operations[].id")
do
  echo "Checking operation $YDB_ID"
  READY=$(get_ready $YDB_ID)
  if [ "$READY" == "true" ]; then
    STATUS=$(get_status $YDB_ID)
    if [ "$STATUS" == "SUCCESS" ] || [ "$STATUS" == "CANCELLED" ]; then
      echo "Removing $STATUS operation $YDB_ID"
      $YA_BIN operation forget $YDB_ID
    fi
  fi
done

NOW=$(date '+%Y-%m-%d')
echo "Scheduling new backup for $NOW"

NEW_OPERATION=$($YA_BIN export s3 --s3-endpoint s3.mds.yandex.net --bucket $S3_BUCKET --exclude export.+ --exclude security.+ --item source=$YDB_DIR,destination=$S3_PATH/$NOW --format proto-json-base64)

NEW_OPERATION_ID=$(echo $NEW_OPERATION | $JQ -r ".id")
echo "Waiting for operation $NEW_OPERATION_ID to complete"

while :
do
  sleep 5
  READY=$(get_ready $NEW_OPERATION_ID)
  if [ "$READY" == "true" ]; then
    echo "Operation $NEW_OPERATION_ID is ready"
    break
  else
    echo "Operation $NEW_OPERATION_ID is not ready, sleeping 5 seconds..."
  fi
done

STATUS=$(get_status $NEW_OPERATION_ID)
if [ "$STATUS" == "SUCCESS" ]; then
  echo "Removing complete operation $NEW_OPERATION_ID"
  $YA_BIN operation forget $NEW_OPERATION_ID
fi


echo "Removing old files from $S3_BUCKET/$S3_PATH..."

mkdir -p ../$S3_BUCKET
chmod +x $GEESEFS
$GEESEFS --endpoint https://s3.mds.yandex.net $S3_BUCKET ../$S3_BUCKET

mkdir -p ../$S3_BUCKET/$S3_PATH
cd ../$S3_BUCKET/$S3_PATH
ls -al .
(ls -r . | tail -n +3 | xargs rm -r --) || echo "Nothing to remove"
