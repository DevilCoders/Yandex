#!/bin/bash
set -e

SECRET_PATH=$1
if [[ -z ${SECRET_PATH} ]]; then
  echo "<SECRET_PATH> does not specified"
  echo ${USAGE}
  exit 1
fi

helm secrets dec ${SECRET_PATH?}

mv ${SECRET_PATH?}.dec ${SECRET_PATH?}

#helm secrets enc ${SECRET_PATH?}
