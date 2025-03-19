#!/usr/bin/env bash

function usage() {
  echo "Usage:"
  echo
  echo "  $0 sec-id create name folder_id"
  echo "  $0 sec-id update name certificate_id "
  echo
}

case "$1" in
"help"|"--help"|"-h"|"")
  echo "Copy certificate from Ya.Vault to Certificate Manager."
  echo 
  usage
  exit 0
  ;;
esac

set -euo pipefail
YAV_SECRET_ID="$1"
ACTION="$2"
NAME="$3"
FOLDER_ID="$4"
CERT_ID="$4"

case "$4" in
fd3* | aoe*)
  YCP_PROFILE=preprod
  ;;
fpq* | b1g*)
  YCP_PROFILE=prod
  ;;
esac

# CM API:
#
# message CreateCertificateRequest {
#   string folder_id = 1 [(required) = true, (length) = "<=50"];
#
#   string name = 2 [(pattern) = "|[a-z]([-a-z0-9]{0,61}[a-z0-9])?"];
#   string description = 8 [(length) = "<=1024"];
#
#   string certificate = 4 [(length) = "<=32768"];
#   string chain = 5 [(length) = "<=2097152"];
#   string private_key = 6 [(required) = true, (length) = "1-524288"];
# }
# 
# message UpdateCertificateRequest {
#   string certificate_id = 1 [(required) = true, (length) = "<=50"];
#   google.protobuf.FieldMask update_mask = 2 [(required) = true];
#
#   string name = 3 [(pattern) = "|[a-z]([-a-z0-9]{0,61}[a-z0-9])?"];
#   string description = 9 [(length) = "<=1024"];
#
#   string certificate = 5 [(length) = "<=32768"];
#   string chain = 6 [(length) = "<=2097152"];
#   string private_key = 7 [(length) = "<=524288"];
# }

REQUEST=$(jq -n --arg name "$NAME" '{name:$name}')

case "$ACTION" in 
"create")
  REQUEST=$(jq --arg x "$FOLDER_ID" '.folder_id=$x' <<< "$REQUEST")
  ;;
"update")
  REQUEST=$(jq --arg x "$CERT_ID" '.certificate_id=$x' <<< "$REQUEST")
  REQUEST=$(jq '.update_mask={paths:["name","chain","private_key"]}' <<< "$REQUEST")
  ;;
*)
  echo "Invalid action $ACTION"
  echo 
  usage
  exit 1
esac

YAV_KEY_NAMES=$(ya vault get version "${YAV_SECRET_ID}" --json | jq -r '.value|keys[]')
YAV_CRT_KEY_NAME=$(grep -E '.*c.*r.*te?$' <<< "$YAV_KEY_NAMES")
YAV_KEY_KEY_NAME=$(grep -E 'key$' <<< "$YAV_KEY_NAMES")

REQUEST=$(jq \
  --arg crt "$(ya vault get version "${YAV_SECRET_ID}" -o "${YAV_CRT_KEY_NAME}")" \
  --arg key "$(ya vault get version "${YAV_SECRET_ID}" -o "${YAV_KEY_KEY_NAME}")" \
  '.chain=$crt|.private_key=$key' <<< "$REQUEST")

echo "This will $ACTION the certificate with"
echo
echo "$REQUEST" | jq -r .chain | openssl x509 -serial -dates -ext subjectAltName -nocert
echo
if [[ "$ACTION" = "update" ]]; then
  echo -n "Current cert is: "
  OLD_CERT=$(ycp --profile="$YCP_PROFILE" certificatemanager v1 certificate get "$CERT_ID")
  yq -r '.name + ", serial=" + .serial' <<< "$OLD_CERT"

  echo "SAN diff (old vs new):"
  diff -y <(yq -r '.domains[]' <<< "$OLD_CERT" | sort) <(jq -r .chain <<< "$REQUEST" | openssl x509 -ext subjectAltName -nocert | grep DNS | sed 's/\s*DNS://g;s/,/\n/g' | sort)
fi

if [[ "${DRY_RUN:-}" != "" ]]; then
  exit 0
fi

read -p 'Type "yes" to proceed: ' -r

if [[ "$REPLY" != "yes" ]]; then
  echo "Aborting"
  exit 0
fi

ycp --profile="$YCP_PROFILE" certificatemanager v1 certificate "$ACTION" -r- <<< "$REQUEST"
