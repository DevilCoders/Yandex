#!/usr/bin/env bash
set -eo pipefail

# You must invoke this script in already terraform-initialized directory.
action=""
zone=""
debug=false
sure=false

usage() {
  echo "$(basename ${0}) -a|--action {enable|disable|check|inflight} -z|--zone {vla|sas|myt} [-d|--debug] [-y|--yes]"
  exit -1
}

while test $# != 0; do
  case "$1" in
  "-a" | "--action")
    action="$2"
    shift
    ;;
  "-z" | "--zone")
    zone="$2"
    shift
    ;;
  "-d" | "--debug") debug=true ;;
  "-y" | "--yes") sure=true ;;
  *) usage ;;
  esac
  shift
done

case "${zone}" in
"vla" | "sas" | "myt") ;;
*) usage ;;
esac

cmd=""
case "${action}" in
"enable")
  cmd='curl -X POST -k https://localhost:7890'
  ;;
"disable")
  cmd='curl -X POST -k https://localhost:7890/?state=DOWN'
  ;;
"check")
  cmd='curl -X GET -k https://localhost:7890'
  ;;
"inflight")
  cmd='curl -X GET -k https://localhost:7890/inflight | jq -jr ".inflight"'
  ;;
*) usage ;;
esac

${debug} && echo "Will ${action} zone \"${zone}\""

cgroup="cloud_preprod_ycf_router_${zone}"
${debug} && echo "Will ${action} cgroup \"${cgroup}\""

case ${sure} in
false)
  case "${action}" in
  "check" | "inflight") ;;
  *)
    read -p "Are you sure? (N/y)" cmd_ack
    cmd_ack=${cmd_ack:0:1}
    case "${cmd_ack}" in
    "y" | "Y") ;;
    *)
      echo "Aborting"
      exit 0
      ;;
    esac
    ;;
  esac
  ;;
*) ${debug} && echo "Continue as '-y' is used" ;;
esac

pssh run --bastion-host bastion.cloud.yandex.net -a --format json "${cmd}" C@${cgroup} | jq -er '.|  map({status:.stdout, host:.hosts[]}) | map({ key: .host, value: .status }) | from_entries'
