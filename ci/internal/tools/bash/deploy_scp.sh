#!/bin/bash

function main_func() {
  CONNECTION=$1
  shift
  FILENAME=$1
  shift
  echo "scp $FILENAME $SSH_FLAGS root@$CONNECTION:~/"
  scp $FILENAME $SSH_FLAGS "root@$CONNECTION:~/"
}

. $(dirname "$0")/_include.sh
