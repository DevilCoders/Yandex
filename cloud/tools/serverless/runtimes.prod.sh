#!/bin/bash

set -e

S3_SECRETS_ID="sec-01ddjbypvysqn9gppkexp80dn5"

debug=true
setup=prod
release=v9222-cba313c

get_aws_ep() {
  case "${1}" in
  "preprod") echo "https://storage.cloud-preprod.yandex.net" ;;
  "prod") echo "https://storage.yandexcloud.net" ;;
  *) exit -1 ;;
  esac
}

get_aws_bucket() {
  case "${1}" in
  "preprod" | "prod") echo "runtime" ;;
  *) exit -1 ;;
  esac
}

get_aws_path() {
  case "${1}" in
  "preprod" | "prod") echo "${2}" ;;
  *) usage ;;
  esac
}

get_yc_profile() {
  case "${1}" in
  "preprod") echo "preprod" ;;
  "prod") echo "prod" ;;
  *) exit -1 ;;
  esac
}

get_yc_sf_ep() {
  case "${1}" in
  "preprod") echo "${yc_sf_ep}" ;;
  "prod") echo "serverless-functions.private-api.ycp.cloud.yandex.net:443" ;;
  *) exit -1 ;;
  esac
}

aws_ep=$(get_aws_ep "${setup}")
aws_bucket=$(get_aws_bucket "${setup}")
aws_path=$(get_aws_path "${setup}" "${release}")
yc_profile=$(get_yc_profile "${setup}")
yc_token=$(yc --profile "${yc_profile}" iam create-token)
yc_sf_ep=$(get_yc_sf_ep "${setup}")
AWS_ACCESS_KEY_ID=$(ya vault get version "${S3_SECRETS_ID}" -o "${setup}_access_key")
AWS_SECRET_ACCESS_KEY=$(ya vault get version "${S3_SECRETS_ID}" -o "${setup}_secret_key")

${debug} && echo "will create runtime ${name} at ${setup} stand with ${release} release"
${debug} && echo "aws_ep:     ${aws_ep}"
${debug} && echo "aws_bucket: ${aws_bucket}"
${debug} && echo "aws_path:   ${aws_path}"
${debug} && echo "yc_profile: ${yc_profile}"
${debug} && echo "yc_sf_ep:   ${yc_sf_ep}"
${debug} && echo "aws access: ${AWS_ACCESS_KEY_ID}"

#RUNTIMES=$(ycp --format json --profile ${yc_profile} serverless functions inner control list-runtime-packages | jq -rc '.[] | .runtime')
#${debug} && echo "runtimes: ${RUNTIMES}"
#echo $RUNTIMES
#exit 0


cat runtimes.${setup}.yaml | yq '.' -  | jq -c '.[]' | while read json; do
  runtime=$(echo "${json}" | jq --raw-output ".runtime")
  name=$(echo "${json}" | jq --raw-output ".image.object_name")
  size=$(AWS_ACCESS_KEY_ID="${AWS_ACCESS_KEY_ID}" AWS_SECRET_ACCESS_KEY="${AWS_SECRET_ACCESS_KEY}" \
      aws s3 ls --endpoint-url="${aws_ep}" "s3://${aws_bucket}/${aws_path}/${name}" | awk '{print $3}')

  echo "name=${name}, json=${json}, size=${size}"
  json=`echo ${json} | jq -c ".image += {bucket_name:\"runtime\", object_name:\"${aws_path}/${name}\", size:$size}"`

  if echo "{\"runtime\":\"${runtime}\"}" | ycp --profile ${yc_profile} serverless functions inner control get-runtime-package -r - > /dev/null 2>&1; then
    echo "update=${json}"
    echo ${json} | ycp --profile ${yc_profile} serverless functions inner control update-runtime-package  -r -
    # grpcurl -H "Authorization: Bearer ${yc_token}" \
    #     -d "${json}" \
    #     "${yc_sf_ep}" \
    #     yandex.cloud.priv.serverless.functions.v1.inner.ControlService/UpdateRuntimePackage
  else
    json=`echo ${json} | jq -c ". += {visibility:\"PRIVATE\"}"`
    echo "create=${json}"
    echo ${json} | ycp --profile ${yc_profile} serverless functions inner control create-runtime-package  -r -
    # grpcurl -H "Authorization: Bearer ${yc_token}" \
    #     -d "${json}" \
    #     "${yc_sf_ep}" \
    #     yandex.cloud.priv.serverless.functions.v1.inner.ControlService/CreateRuntimePackage
  fi

done
