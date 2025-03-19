#!/usr/bin/env bash
# shellcheck disable=SC2155

# Fail fast.
set -e
set -o pipefail

############
# Preamble #
############
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color
BOOTSTRAP_HOST=${BOOTSTRAP_HOST:=bootstrap.cloud.yandex.net}
TICKET=${TICKET:?"TICKET env variable is required"}
# https://oauth.yandex-team.ru/authorize?response_type=token&client_id=5f671d781aca402ab7460fde4050267b
export ST_OAUTH_TOKEN_FILE=${ST_OAUTH_TOKEN_FILE:=~/.config/oauth/st}
# or use an arcadia root relative path (the root is a parent ir containing "$A/.arcadia.root")
THIS_SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ARC_IAM_SCRIPTS=${THIS_SCRIPT_DIR?}
source "${ARC_IAM_SCRIPTS}/juggler_downtimes.sh"
source "$ARC_IAM_SCRIPTS/get_host_env.sh"
HOST_FQDN=${1}

if [[ "$DRY_RUN" = "y" ]]; then
    echo -e "${RED?}DRY RUN is enabled! No actual cloud operations will be executed${NC?}"
    OPTIONAL_DRY_RUN="echo DRY_RUN "
fi

if [[ -z ${XDG_CURRENT_DESKTOP} ]]; then
  url_open=open
else
  url_open=xdg-open
fi

if [[ -z ${HOST_FQDN} ]]; then
  echo "Usage: TICKET=CLOUD-12345 $0 <host fqdn>"
  exit 64 # EX_USAGE /* command line usage error */
fi

####################
# Helper functions #
####################

# Executes a command and saves the output to the ticket
function log_to_ticket {
  title=${1}
  cmd=${2}

  echo -e "${GREEN?}${title?}${NC?}"
  echo "Command:    ${cmd?}"

  # an important part is to run the command in this process as it may modify environment variables (e.g. juggler tools),
  # so no command substitution or piped processing is available
  local tmp_file=$(mktemp eval_log.XXXXXX)
  set +e
  eval "${OPTIONAL_DRY_RUN} ${cmd?}" > "${tmp_file?}" 2>&1
  local rc=$?
  set -e
  output=$(cat "${tmp_file?}")
  rm "${tmp_file?}"

  echo "${output}"
  if (( rc != 0 ));
  then
    echo -e "${RED?}The command has failed with error code ${rc?}!${NC?}"
    exit ${rc?}
  fi

  comment_log=$(echo "${output}" | sed $'s/\x1B\[[0-9;]*[A-Za-z]//g') # uncolor
  comment=$(printf '<{ %s for %s\n**Command**:\n```%s```\n**Output:**\n<[\n%s\n]>\n}>' "${title?}" "${HOST_FQDN?}" "${cmd?}" "${comment_log?}")
  "${ARC_IAM_SCRIPTS?}/add_startrek_comment.sh" "${TICKET?}" "${comment?}"
}

# Identifies the Solomon cluster by environment.
function get_solomon_ds {
  case $1 in
    preprod|testing) echo Solomon%20Cloud%20Preprod ;;
    israel)          echo Solomon%20Cloud%20IL      ;;
    *)               echo Solomon%20Cloud%20Prod    ;;
  esac
}

function get_solomon_cluster {
  case $1 in
    internal-prod)  echo prod-internal  ;;
    *)              echo "${1?}"        ;;
  esac
}

# Resolves the service cloud for the environment.
function get_service_cloud {
  case $1 in
    internal-prod)       echo b1goor5a9cm1meamkto3 ;;
    internal-prestable)  echo b1g3bl7j66p38qhmbbjo ;;
    internal-dev)        echo b1gvqc1bvr7go27er7df ;;
    *)                   echo yc.iam.service-cloud ;;
  esac
}

# Builds YCP CLI call prefix with the right profile and returns the result in the JSON format.
function ycp_alias {
  echo ycp --profile "${1:-${profile?}}" --format json
}

# Scans all folders in the service cloud and looks for an instance matching the host FQDN.
function get_host_instance_id() {
    local fqdn=${1}
    local name=${fqdn/[.]*/}
    local instance_id
    local folders=($($(ycp_alias) resource-manager folder list --cloud-id "${service_cloud?}" | jq -rc .[].id))
    for folder in "${folders[@]}"
    do
        instance_id=$($(ycp_alias) compute instance list --folder-id "${folder?}" --filter "name='${name?}'" |
          jq -rc "map(select(.fqdn==\"${fqdn?}\")) | .[].id")
        if [[ -n "${instance_id}" ]]; then
            echo "${instance_id}"
            break
        fi
    done
}

function get_instance_status() {
    local instance_id=${1}
    $(ycp_alias) compute instance get $instance_id | jq -r '.status'
}

function alb_exists() {
    local api_profile="${profile?}"
    local folder_id=$1
    if [[ $api_profile == "testing" ]]; then
        api_profile="preprod"
    fi

    $(ycp_alias ${api_profile?}) platform alb load-balancer list --folder-id "${folder_id?}" | jq "length > 0"
}

function downgrade_dt_to_reboot_count_only() {
  first_dt=${CURRENT_DT?}
  add_downtime "$(dt_filters host "${HOST_FQDN?}" service reboot-count)" 180
  remove_downtime "${first_dt?}"
}

#################################
# The main program starts here. #
#################################

echo "Starting SVM re-deployment, in case of troubles see the old manual instructions: https://stackoverflow.yandex-team.ru/questions/STACKOVERFLOW-518"

host_env=$(get_host_env "${HOST_FQDN?}")
if [[ -z ${host_env} ]]; then
  echo "Can't figure out environment for host ${HOST_FQDN?}"
  exit 1
fi

# All internal SVMs are in prod environment
if [[ "${host_env?}" == internal-* ]]; then
  profile=prod
else
  profile=${host_env?}
fi

echo -e "${GREEN?}Step 0. Collecting SVM info.${NC?}"
service_cloud=$(get_service_cloud "${host_env?}")
instance_id=$(get_host_instance_id "${HOST_FQDN?}")
if [[ -z "${instance_id}" ]]; then
    read -p "No instances for fqdn ${HOST_FQDN?} found in ${service_cloud?}. Continue (y/n)?" CONFIRM
else
	read -p "You are going to delete and re-add SVM ${HOST_FQDN?} (instance ${instance_id?}). Are you sure (y/n)?" CONFIRM
fi

if [[ ! "$CONFIRM" = "y" ]]; then
    echo "Cancelled"
    exit 77 # EX_NOPERM /* permission denied */
fi

log_file=$(date "+.readd-svm-%Y%m%d%H%M%S.log")

log_to_ticket "Step 1. Downtime" "add_downtime ${HOST_FQDN?} 240"

if [[ ! -z "${instance_id}" ]]; then
	if [[ $(get_instance_status $instance_id) != "ERROR" ]] ; then
    log_to_ticket "Step 2.1. Stop services" "pssh run 'sudo systemctl stop nginx.service yc-*.service || true' ${HOST_FQDN?}"
    log_to_ticket "Step 2.2. Stop SVM" "$(ycp_alias) compute instance stop ${instance_id?}"
	fi

  log_to_ticket "Step 3. Delete SVM" "$(ycp_alias) compute instance delete ${instance_id?}"
else
  echo "Skipping steps 2 and 3. No running SVM found"
fi

# fix template name for preprod, if required
if [[ ${profile?} == "preprod" ]]; then
    bootstrap_template="pre-prod"
else
    bootstrap_template="${profile?}"
fi

echo "(the next step takes a while, be patient while yc-bootstrap add-svms is running; logs: ${log_file?})"
cmd="yc-bootstrap --template ${bootstrap_template?}.yaml --ticket-id ${TICKET?} --filter host=${HOST_FQDN?} --apply add-svms --skip-confirm"
echo "... running:   ${cmd?}"
pssh run "nohup ${OPTIONAL_DRY_RUN} ${cmd?} | tee -a ${log_file?}" "${BOOTSTRAP_HOST?}"
pssh scp "${BOOTSTRAP_HOST?}:${log_file?}" "${log_file?}"

log_to_ticket "Step 4. Re-add SVM" "tail -n 30 ${log_file?}"

echo -e "${GREEN?}Step 5. Checking the new instance.${NC?}"
echo "... instance state check"
instance_id=$(get_host_instance_id "${HOST_FQDN?}")
if [[ -z "${instance_id}" ]]; then
    echo "Failed to find new instance id for fqdn ${HOST_FQDN?} in ${service_cloud?}"
    exit 3
fi

echo "... ALB presence check"
folder_id=$($(ycp_alias) compute instance get "${instance_id?}" | jq -rc ".folder_id")
found_alb=$(alb_exists "${folder_id?}")

if [[ ${found_alb} == true ]]; then
  echo -e "${RED?}Do not forget to update ALB!${NC?}"
  "${url_open}" "https://a.yandex-team.ru/arc/trunk/arcadia/cloud/iam/terraform/alb/README.md"
fi

echo "... misc. instance info and statuses"
log_to_ticket "Instance info" "$(ycp_alias) compute instance get ${instance_id?}"

log_to_ticket "yc services status" "pssh run \"sudo systemctl list-units --all --type=service nginx.service yc-*.service\" ${HOST_FQDN?}"

log_to_ticket "monrun output" "pssh run monrun ${HOST_FQDN?}"

solomon_ds=$(get_solomon_ds "${host_env?}")
solomon_cluster=$(get_solomon_cluster "${host_env?}")
host_name=${HOST_FQDN/[.]*/}
read -p "press any key to visit grafana page for ${host_name?}"
echo "" # a new line after read -p

"${url_open}" "https://grafana.yandex-team.ru/d/iam-duty-deploy/iam-duty-deploy?orgId=1&refresh=1m&from=now-1h&to=now&var-service=All&var-appFilter=*_server&var-ds=${solomon_ds?}&var-cluster=${solomon_cluster?}&var-host=${host_name?}"

log_to_ticket "Step 6. Clearing the initial total downtime and muting the reboot-count alert for 3 more hours" "downgrade_dt_to_reboot_count_only"
