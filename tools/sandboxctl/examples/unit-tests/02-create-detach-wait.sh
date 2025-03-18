#!/bin/bash

set -e

DIR=$(realpath $(dirname $0))
BIN="$DIR/../../bin/sandboxctl $SANDBOXCTL_XARGS"

TASK_ID=$($BIN create  -q -N TEST_TASK)
echo $TASK_ID
$BIN get_task --wait $TASK_ID
