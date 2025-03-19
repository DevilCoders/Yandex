#!/bin/sh -e

ACTION=${1:-local}
SCOPE=${2:-test}
LOOKUP_DIR="${3:-.}"

SCRIPT_DIR="$( cd "$(dirname "$0")" ; pwd -P )" #"
DIR=$SCRIPT_DIR/src/$SCOPE/resources/dashboard

for f in $(cd $DIR; find "$LOOKUP_DIR" -name '*.yaml' | fgrep -vi 'include/'); do
  #if [ "$(basename $f)" != "errors.yaml" ]; then
    ./spec-single.sh $ACTION $SCOPE $f
    read -p "Press a key to continue"
  #fi
done
