#!/bin/bash

S3_SECRETS_ID="sec-01ddjbypvysqn9gppkexp80dn5"

action=""
setup=""
release=""
debug=false

get_aws_ep() {
  local setup="${1}"
  case "${setup}" in
  "dev") echo "https://storage.cloud-preprod.yandex.net" ;;
  "preprod") echo "https://storage.cloud-preprod.yandex.net" ;;
  "prod") echo "https://storage.yandexcloud.net" ;;
  *) exit -1 ;;
  esac
}

get_aws_bucket() {
  case "${1}" in
  "dev") echo "serverless-dev-storage" ;;
  "preprod" | "prod") echo "runtime" ;;
  *) exit -1 ;;
  esac
}

get_aws_path() {
  local setup="${1}"
  local release="${2}"
  local bucket=""
  local s3_path=""
  case "${setup}" in
  "dev") echo "releases/${release}/runtime" ;;
  "preprod" | "prod") echo "${release}" ;;
  *) usage ;;
  esac
}

get_yc_profile() {
  case "${1}" in
  "dev") echo "preprod" ;;
  "preprod") echo "preprod" ;;
  "prod") echo "prod" ;;
  *) exit -1 ;;
  esac
}

get_functions_ep() {
  case "${1}" in
  "dev") echo "functions-dev.private-api.ycp.cloud-preprod.yandex.net" ;;
  "preprod") echo "serverless-functions.private-api.ycp.cloud-preprod.yandex.net" ;;
  "prod") echo "serverless-functions.private-api.ycp.cloud.yandex.net" ;;
  *) exit -1 ;;
  esac
}

usage() {
  echo "$(basename ${0}) -a|--action {create|copy|update} -s|--setup {preprod|prod} -r|--release RELEASE_VERSION [-d|--debug]"
  exit -1
}

create_runtime() {
  aws_ep=$(get_aws_ep "${setup}")
  aws_bucket=$(get_aws_bucket "${setup}")
  aws_path=$(get_aws_path "${setup}" "${release}")
  yc_profile=$(get_yc_profile "${setup}")
  yc_token=$(yc --profile "${yc_profile}" iam create-token)
  AWS_ACCESS_KEY_ID=$(ya vault get version "${S3_SECRETS_ID}" -o "${setup}_access_key")
  AWS_SECRET_ACCESS_KEY=$(ya vault get version "${S3_SECRETS_ID}" -o "${setup}_secret_key")
  functions_ep=$(get_functions_ep "${setup}")

  ${debug} && echo "will create runtime ${name} at ${setup} stand with ${release} release"
  ${debug} && echo "aws_ep:     ${aws_ep}"
  ${debug} && echo "aws_bucket: ${aws_bucket}"
  ${debug} && echo "aws_path:   ${aws_path}"
  ${debug} && echo "yc_profile: ${yc_profile}"
  ${debug} && echo "aws access: ${AWS_ACCESS_KEY_ID}"
  ${debug} && echo "cpl ep:     ${functions_ep}"

  size=$(AWS_ACCESS_KEY_ID="${AWS_ACCESS_KEY_ID}" AWS_SECRET_ACCESS_KEY="${AWS_SECRET_ACCESS_KEY}" \
    aws s3 ls --endpoint-url="${aws_ep}" "s3://${aws_bucket}/${aws_path}/${name}" | awk '{print $3}')
  data=$(grpcurl -H "Authorization: Bearer ${yc_token}" \
    -d "{\"runtime\": \"${name}\",\"image\":{\"bucket_name\":\"${aws_bucket}\",\"object_name\":\"${aws_path}/${name}\",\"size\":${size}},\"default_builder\":\"none\"}" \
    "${functions_ep}:443" \
    yandex.cloud.priv.serverless.functions.v1.inner.ControlService/CreateRuntimePackage 2>/dev/null)
  if [[ $? -ne 0 ]]; then
    echo "FAILED"
  else
    echo "OK"
    ${debug} && (echo -n "${data}" | jq)
  fi
}

copy_runtime() {
  copy_from=""
  copy_to=""

  case "${setup}" in
  "preprod")
    copy_from="dev"
    copy_to="preprod"
    ;;
  "prod")
    copy_from="preprod"
    copy_to="prod"
    ;;
  *) usage ;;
  esac

  workdir=$(mktemp -d)
  ${debug} && echo "using ${workdir}"

  from_aws_ep=$(get_aws_ep "${copy_from}")
  from_aws_bucket=$(get_aws_bucket "${copy_from}")
  from_path=$(get_aws_path "${copy_from}" "${release}")
  from_AWS_ACCESS_KEY_ID=$(ya vault get version "${S3_SECRETS_ID}" -o "${copy_from}_access_key")
  from_AWS_SECRET_ACCESS_KEY=$(ya vault get version "${S3_SECRETS_ID}" -o "${copy_from}_secret_key")

  to_aws_ep=$(get_aws_ep "${copy_to}")
  to_aws_bucket=$(get_aws_bucket "${copy_to}")
  to_path=$(get_aws_path "${copy_to}" "${release}")
  to_AWS_ACCESS_KEY_ID=$(ya vault get version "${S3_SECRETS_ID}" -o "${copy_to}_access_key")
  to_AWS_SECRET_ACCESS_KEY=$(ya vault get version "${S3_SECRETS_ID}" -o "${copy_to}_secret_key")

  mkdir ${workdir}/${release}

  ${debug} && echo "will copy from s3://${from_aws_bucket}/${from_path}/"

  AWS_ACCESS_KEY_ID=${from_AWS_ACCESS_KEY_ID} AWS_SECRET_ACCESS_KEY=${from_AWS_SECRET_ACCESS_KEY} \
    aws s3 --endpoint-url="${from_aws_ep}" sync \
    "s3://${from_aws_bucket}/${from_path}/" ${workdir}/${release}
  ${debug} && (ls -lR ${workdir}/${release})

  ${debug} && echo "will copy to s3://${to_aws_bucket}/${to_path}/"

  AWS_ACCESS_KEY_ID=${to_AWS_ACCESS_KEY_ID} AWS_SECRET_ACCESS_KEY=${to_AWS_SECRET_ACCESS_KEY} \
    aws s3 --endpoint-url="${to_aws_ep}" sync \
    ${workdir}/${release} "s3://${to_aws_bucket}/${to_path}/"

  rm -rf ${workdir}
}

update_runtime() {
  aws_ep=$(get_aws_ep "${setup}")
  aws_bucket=$(get_aws_bucket "${setup}")
  aws_path=$(get_aws_path "${setup}" "${release}")
  yc_profile=$(get_yc_profile "${setup}")
  yc_token=$(yc --profile "${yc_profile}" iam create-token)
  AWS_ACCESS_KEY_ID=$(ya vault get version "${S3_SECRETS_ID}" -o "${setup}_access_key")
  AWS_SECRET_ACCESS_KEY=$(ya vault get version "${S3_SECRETS_ID}" -o "${setup}_secret_key")
  functions_ep=$(get_functions_ep "${setup}")

  ${debug} && echo "will update ${setup} stand with ${release} release"
  ${debug} && echo "aws_ep:     ${aws_ep}"
  ${debug} && echo "aws_bucket: ${aws_bucket}"
  ${debug} && echo "aws_path:   ${aws_path}"
  ${debug} && echo "yc_profile: ${yc_profile}"
  ${debug} && echo "aws access: ${AWS_ACCESS_KEY_ID}"
  ${debug} && echo "cpl ep:     ${functions_ep}"

  set -f
  IFS=$'\n'
  for runtime_info in \
    $(AWS_ACCESS_KEY_ID="${AWS_ACCESS_KEY_ID}" AWS_SECRET_ACCESS_KEY="${AWS_SECRET_ACCESS_KEY}" \
      aws s3 ls --endpoint-url="${aws_ep}" "s3://${aws_bucket}/${aws_path}/"); do
    runtime=$(echo -n ${runtime_info} | awk '{print $4}')
    size=$(echo -n ${runtime_info} | awk '{print $3}')
    echo -n "updating ${runtime} with size ${size}: "
    data=$(grpcurl -H "Authorization: Bearer ${yc_token}" \
      -d "{\"runtime\": \"${runtime}\",\"image\":{\"bucket_name\":\"${aws_bucket}\",\"object_name\":\"${aws_path}/${runtime}\",\"size\":${size}},\"default_builder\":\"none\"}" \
      "${functions_ep}:443" \
      yandex.cloud.priv.serverless.functions.v1.inner.ControlService/UpdateRuntimePackage 2>/dev/null)
    if [[ $? -ne 0 ]]; then
      echo "FAILED (skipping)"
    else
      echo "OK"
      ${debug} && (echo -n "${data}" | jq '.')
    fi
  done
  set +f
  unset IFS
}

while test $# != 0; do
  case "$1" in
  "-a" | "--action")
    action="$2"
    shift
    ;;
  "-s" | "--setup")
    setup="$2"
    shift
    ;;
  "-r" | "--release")
    release="$2"
    shift
    ;;
  "-d" | "--debug") debug=true ;;
  *) usage ;;
  esac
  shift
done

case "${setup}" in
"preprod" | "prod") ;;
*) usage ;;
esac

if [[ -z "${release}" ]]; then
  usage
fi

case "${action}" in
"copy") copy_runtime ;;
*) usage ;;
esac
