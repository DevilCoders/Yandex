#!/bin/bash

NLB_ID=b7r6gqtf2lguuu4fgho2
CREATE_SPEC="nlb-grpc-dpl-spec.json"

case $1 in
create)
  ycp load-balancer network-load-balancer create --request "$CREATE_SPEC" --profile prod
  ;;
update)
  TMP_DIR=`mktemp -d`
  trap "rm -r ${TMP_DIR}" EXIT
  UPDATE_SPEC="$TMP_DIR/update-spec.json"
  cat "$CREATE_SPEC" | jq 'del(.folder_id, .type, .region_id)' > "$UPDATE_SPEC"
  ycp load-balancer network-load-balancer update --id "$NLB_ID" --request "$UPDATE_SPEC" --profile prod
  ;;
*)
  echo "Usage: ./nlb-grpc-dpl.sh create|update" >&2
  exit 1
  ;;

esac
