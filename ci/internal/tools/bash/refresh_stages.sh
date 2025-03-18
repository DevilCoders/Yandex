#!/bin/bash

set -e
FILENAME="_stages.sh"

echo '#!/bin/bash

# GENERATED AUTOMATICALLY BY refresh_stages.sh
case $STAGE in' > $FILENAME

ALL_STAGES=""

function run() {
  STAGE=$1
  UNIT=$2
  echo "Processing stage $1"
  TMP_STAGE="stage.yaml"
  TMP_CASE="case.txt"
  ya tool dctl get stage "$STAGE" > $TMP_STAGE
  if [[ -z "$UNIT" ]]; then
    extract_boxes -i $TMP_STAGE -o $TMP_CASE
  else
    extract_boxes -i $TMP_STAGE -u $UNIT -o $TMP_CASE
  fi
  cat $TMP_CASE >> $FILENAME
  rm stage.yaml
  rm $TMP_CASE

  ALL_STAGES="$ALL_STAGES$STAGE "
}

function test() {
  run ci-api-testing
  run ci-ayamler-api-testing
  run ci-event-reader-testing
  run ci-observer-api-testing
  run ci-observer-reader-testing
  run ci-storage-api-testing
  run ci-storage-exporter-testing
  run ci-storage-post-processor-testing
  run ci-storage-reader-testing
  run ci-storage-shard-testing
  run ci-storage-tms-testing
  run ci-tms-testing
}

function prestable() {
  run ci-storage-api-prestable
  run ci-storage-post-processor-prestable
  run ci-storage-reader-prestable
  run ci-storage-shard-prestable
  run ci-storage-tms-prestable
}

function stable() {
  run ci-api-stable
  run ci-ayamler-api-stable
  run ci-event-reader-stable
  run ci-observer-api-stable
  run ci-observer-reader-stable
  run ci-storage-api-stable
  run ci-storage-post-processor-stable
  run ci-storage-reader-stable
  run ci-storage-shard-stable
  run ci-storage-tms-stable
  run ci-tms-stable
}

function testenv() {
    run testenv-prod engine
}

test
prestable
stable
testenv

echo '*)
    UNIT=""
    BOX=""
    ;;
esac

export STAGE
export UNIT
export BOX
export ALL_STAGES=('$ALL_STAGES')
' >> $FILENAME
