#!/bin/bash
set -e

STAND="preprod"

RED="\e[91m"
NC="\e[39m"
echo -e "${RED}DO NOT USE THIS SCRIPT ANYMORE${NC}!!!"
echo -e "${RED}USE${NC} 'datacloud/bootstrap/terraform/3_deployments/${STAND?}/<CLOUD_SERVICE_PROVIDER>/<IAM_SERVICE>': 'terraform plan/apply' ${RED}instead${NC}!!!"

USAGE="usage: $0 <ACTION> <CLOUD_SERVICE_PROVIDER> <CHART_TYPE> <IAM_SERVICE>"

ACTION=$1
if [[ -z ${ACTION} ]]; then
  echo "<ACTION> does not specified"
  echo ${USAGE}
  exit 1
fi
if [[ ! " install upgrade update " =~ " ${ACTION} " ]]; then
  echo '<ACTION> must be one of: "install", "upgrade", "update"'
  echo ${USAGE}
  exit 1
fi
if [[ ${ACTION} == "update" ]]; then
  ACTION="upgrade"
fi

CLOUD_SERVICE_PROVIDER=$2
if [[ -z ${CLOUD_SERVICE_PROVIDER} ]]; then
  echo "<CLOUD_SERVICE_PROVIDER> does not specified"
  echo ${USAGE}
  exit 1
fi
if [[ ! " aws yc " =~ " ${CLOUD_SERVICE_PROVIDER} " ]]; then
  echo '<CLOUD_SERVICE_PROVIDER> must be one of: "aws", "yc"'
  echo ${USAGE}
  exit 1
fi

CHART_TYPE=$3
if [[ -z ${CHART_TYPE} ]]; then
  echo "<CHART_TYPE> does not specified"
  echo ${USAGE}
  exit 1
fi
if [[ ! " deployment common load-balancer iam-sync populate-db " =~ " ${CHART_TYPE} " ]]; then
  echo '<CHART_TYPE> must be one of: "deployment", "common", "load-balancer", "iam-sync", "populate-db"'
  echo ${USAGE}
  exit 1
fi

ADD_VARS_FILE=""
ADD_SECRETS_FILE=""
CHART_NAME=${CHART_TYPE?}
if [[ ${CHART_TYPE?} == "load-balancer" ]]; then
  CHART_NAME="load-balancers"
  ADD_VARS_FILE="-f datacloud-${CHART_TYPE?}/${STAND?}/${CLOUD_SERVICE_PROVIDER?}/terraform_values.yaml"
#elif [[ ${CHART_TYPE?} == "common" ]] || [[ ${CHART_TYPE?} == "iam-sync" ]] || [[ ${CHART_TYPE?} == "populate-db" ]]; then
#  ADD_SECRETS_FILE="-f datacloud-${CHART_TYPE?}/${STAND?}/${CLOUD_SERVICE_PROVIDER?}/${CHART_TYPE?}/secrets.yaml"
#  if [[ ${CHART_TYPE?} == "populate-db" ]]; then
#    ADD_VARS_FILE="-f datacloud-${CHART_TYPE?}/${STAND?}/${CLOUD_SERVICE_PROVIDER?}/${CHART_TYPE?}/files/vars.yaml"
#  fi
#elif [[ ${CHART_TYPE?} == "deployment" ]]; then
else
  if [[ ${CHART_TYPE?} == "deployment" ]]; then
    IAM_SERVICE=$4

    if [[ -z ${IAM_SERVICE} ]] ; then
      echo "<IAM_SERVICE> does not specified"
      echo ${USAGE}
      exit 1
    fi
    if [[ ! " access-service iam-control-plane mfa-service openid-server org-service rm-control-plane token-service identity " =~ " ${IAM_SERVICE} " ]]; then
      echo '<IAM_SERVICE> must be one of: "access-service", "iam-control-plane", "mfa-service", "openid-server", "org-service", "rm-control-plane", "token-service", "identity"'
      echo ${USAGE}
      exit 1
    fi
    CHART_NAME=${IAM_SERVICE}
  fi
  ADD_VARS_FILE="-f datacloud-${CHART_TYPE?}/${STAND?}/${CLOUD_SERVICE_PROVIDER?}/${CHART_NAME?}/files/vars.yaml"
  ADD_SECRETS_FILE="-f datacloud-${CHART_TYPE?}/${STAND?}/${CLOUD_SERVICE_PROVIDER?}/${CHART_NAME?}/secrets.yaml"
fi

OPTS=""
if [[ ${5} == "--dry-run" ]] || [[ ${5} == "--dry" ]]; then
    OPTS="${OPTS} --dry-run"
fi

helm secrets ${ACTION?} ${CHART_NAME?} ./datacloud-${CHART_TYPE?}/ ${OPTS} --debug \
      --kubeconfig "../bootstrap/terraform/1_infra/${STAND?}/${CLOUD_SERVICE_PROVIDER?}/.kubeconfig_iam-${STAND?}" \
      -f "datacloud-${CHART_TYPE?}/values.yaml" \
      -f "datacloud-${CHART_TYPE?}/${STAND?}/${CLOUD_SERVICE_PROVIDER?}/values.yaml" \
      ${ADD_VARS_FILE?} \
      ${ADD_SECRETS_FILE}
#      --set namespace=${NAMESPACE?} \
#       --namespace=${NAMESPACE?}

