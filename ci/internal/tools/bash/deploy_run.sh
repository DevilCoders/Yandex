#!/bin/bash

function main_func() {
  CONNECTION=$1
  shift
  echo "ssh $SSH_FLAGS root@$CONNECTION $*"
  ssh $SSH_FLAGS "root@$CONNECTION" "$*"
}

. $(dirname "$0")/_include.sh
