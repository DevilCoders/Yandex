#!/usr/bin/env bash

[ "$IAM_METADATA_ENV_VARS_AVAILABLE" != "yes" ] && echo "please init settings and variables by running \`source ./env.sh\`" && exit 1
OK="\033[0;32mOK\033[0m"
ERR="\033[0;31mNOT OK\033[0m"

function check_aws_s3 {
  echo -n "checking s3 credentials: "
  grep "\[$S3_AWS_PROFILE\]" ~/.aws/credentials >/dev/null 2>/dev/null && echo -e "$OK" ||
    echo -e "$ERR
    Seems like there's no profile [$S3_AWS_PROFILE] in ~/.aws/credentials
    Execute \`init_aws_creds\` function to add the profile
  "
  local cmd="aws --profile $S3_AWS_PROFILE --endpoint-url $S3_ENDPOINT s3 ls s3://$S3_BUCKET"
  echo -n "testing command \`$cmd\`: "
  $cmd >/dev/null && echo -e "$OK" || echo -e "$ERR"
}

function init_aws_creds {
  local secret=$(ycp --profile=prod --format json lockbox v1 payload get --secret-id $S3_CREDS_LOCKBOX_ID) && echo "
[$S3_AWS_PROFILE]
aws_access_key_id = $(echo $secret | jq '.entries[0].text_value' -r)
aws_secret_access_key = $(echo $secret | jq '.entries[1].text_value' -r)
" >>~/.aws/credentials
}

function check_ycp {
   for profile in $STANDS; do
     # First loop - authenticate in all profiles. This will generate a lot of noise (but it's a bad idea to /dev/null it).
     # This noise would make actual checks less readable so generate it first and do actual checks later.
     ycp --profile=$profile iam role get yq.viewer >/dev/null 2>/dev/null
   done

  for profile in $STANDS; do
    echo -n "check ycp profile '$profile' and required permission: "
    local token=$(ycp --profile=$profile iam create-token 2>/dev/null)
    local req="{
      iam_token: $token,
      permission: iam.roles.create,
      resource_path: [{type: iam.gizmo, id: gizmo}]
    }"
    out=$(echo $req | ycp --profile=$profile service-control access authorize -r - 2>&1)
    result=$?
    if [ $result -eq 0 ]; then
      echo -e "$OK"
    else
      echo -e "$ERR $out"
    fi
  done

  echo -n "check ycp version: "
  local min_required="0.3.0-2022-05-12T10-29-00Z"
  local current=$(ycp version | head -n1 | sed 's/Yandex.Cloud Private CLI \([^ ]*\).*/\1/')
  test "$current" \< $min_required && echo -e "$ERR ycp version $min_required or newer is required, current - $current" || echo -e "$current seems $OK"
}

function check_oauth_tokens {
  echo -n "check StarTrek OAuth token: "
  if [ "$ST_OAUTH_TOKEN" ]; then
    echo -e "env variable ST_OAUTH_TOKEN is present $OK"
  elif [ -f "$ST_OAUTH_TOKEN_FILE" ]; then
    echo -e "found file $ST_OAUTH_TOKEN_FILE $OK"
  else
    echo -e "$ERR Please get an OAuth token from $ST_OAUTH_URL and save it to $ST_OAUTH_TOKEN_FILE"
  fi
  echo -n "check Juggler OAuth token: "
  if [ "$JUGGLER_OAUTH_TOKEN" ]; then
    echo -e "env variable JUGGLER_OAUTH_TOKEN is present $OK"
  elif [ -f "$JUGGLER_OAUTH_TOKEN_FILE" ]; then
    echo -e "found file $JUGGLER_OAUTH_TOKEN_FILE $OK"
  else
    echo -e "$ERR Please get an OAuth token from $JUGGLER_OAUTH_URL and save it to $JUGGLER_OAUTH_TOKEN_FILE"
  fi
}

function check_executables {
  echo -n "check jq is present: "
  jq -h 2>/dev/null >/dev/null && echo -e "$OK" || echo -e "$ERR"
  echo -n "check grpcurl is present: "
  grpcurl -h 2>/dev/null >/dev/null && echo -e "$OK" || echo -e "$ERR"
}

function check_cloud_go_repo {
  echo -n "check cloud-go repository in $CLOUD_GO_PATH: "
  test -d "$CLOUD_GO_PATH/.git" && echo -e "$OK" || echo -e "$ERR"
  echo -n "cloud-go is on master branch: "
  local branch=$(git -C $CLOUD_GO_PATH branch --show-current)
  test "$branch" = "master" && echo -e "$OK" || echo -e "$ERR - $branch"
}

function check_access_service_access {
  HOSTS="
    x@iam-as-*.svc.cloud-testing.yandex.net[0]
    x@iam-as-*.svc.cloud-preprod.yandex.net[0]
    x@iam-as-*.svc.cloud.yandex.net[0]
    x@iam-internal-dev-*.svc.cloud.yandex.net[0]
    x@iam-internal-prestable-*.svc.cloud.yandex.net[0]
    x@iam-ya-*.svc.cloud.yandex.net[0]
    x@iam-as-il1-*.svc.yandexcloud.co.il[0]
  "

  for host in $(echo $HOSTS); do
    resolved=$(pssh list -l $host | head -n 1)
    echo -n "check direct network access to $resolved:4286 "
    nc -z -w3 $resolved 4286 && echo -e $OK || echo -e $ERR
  done
}

check_ycp
check_executables
check_oauth_tokens
check_aws_s3
check_cloud_go_repo
check_access_service_access
