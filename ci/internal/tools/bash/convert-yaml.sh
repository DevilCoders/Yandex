#!/bin/bash

set -e

function run() {
  STAGE=$1
  echo "Processing stage $1"
  ya tool dctl get stage $STAGE > $STAGE.yaml
  #convert_yaml -i $STAGE-in.yaml -o $STAGE-out.yaml
  #ya tool dctl put stage $STAGE-out.yaml
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
  run testenv-prod
  run testenv-ui-production
}

function tools() {
  run ci-temporal-testing
  run ci-temporal-prestable
  run ci-temporal-stable
  run ci-health-stable
}

test
prestable
stable
testenv
tools
