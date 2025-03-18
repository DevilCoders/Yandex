#!/bin/bash

function main_func() {
  CONNECTION=$1
  ssh-keygen -R $CONNECTION
}

. $(dirname "$0")/_include.sh
