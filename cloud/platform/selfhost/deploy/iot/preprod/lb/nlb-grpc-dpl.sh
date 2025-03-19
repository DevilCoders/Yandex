#!/bin/bash

NLB_ID=c58dj3red5t10e29aeao
CREATE_SPEC="nlb-grpc-dpl-spec.json"

case $1 in
create)
  ycp load-balancer network-load-balancer create --request "$CREATE_SPEC" --profile preprod
  ;;
update)
  TMP_DIR=`mktemp -d`
  trap "rm -r ${TMP_DIR}" EXIT
  UPDATE_SPEC="$TMP_DIR/update-spec.json"
  cat "$CREATE_SPEC" | jq 'del(.folder_id, .type, .region_id)' > "$UPDATE_SPEC"
  ycp load-balancer network-load-balancer update --id "$NLB_ID" --request "$UPDATE_SPEC" --profile preprod
  ;;
*)
  echo "Usage: ./nlb-grpc-dpl.sh create|update" >&2
  exit 1
  ;;

esac
