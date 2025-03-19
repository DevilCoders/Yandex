#!/usr/bin/env bash

function targets {
  # Usage: targets L3_id TG_id
  echo "
    network_load_balancer_id: $1
    target_group_id: $2
  " | ycp load-balancer network-load-balancer get-target-states "${@:3}" -r- \
    |  yq -c '.target_states[]|del(.subnet_id)'
}

function instances {
  echo IG $1 instances:
  ycp microcosm instance-group list-instances "$@" \
    | yq -c '.[]|[.status,.network_interfaces[]?.primary_v6_address.address,.status_message // "No error"]'
}

echo
echo api-router
echo
echo IPv6:
targets cupsk32vs4878e7ge65u cupgos9qc35nmmilth32 --profile=gpn-l7
echo IPv4:
targets cupsfkp9nkakccak6dcu cupgos9qc35nmmilth32 --profile=gpn-l7
instances ag4hqg685lttlefvlg0l --profile=gpn-l7

echo
echo cpl-router
echo
targets cup9oiboup5bmdn4ehjq cgmr6kqtokda09qla87b --profile=gpn-l7
instances ag47bl3k7lmp7l396f7q --profile=gpn-l7

echo
echo staging
echo
targets cupgtkmvikkfn5b47tdi cup6qq4fmd1p5gv1u7qh --profile=gpn-l7
instances ag470vr8hbq2bk0k651f --profile=gpn-l7

echo
echo alb
echo
targets cup47oinkb2dllee0qp1 cgmbqtl7no1odc0vauq3 --profile=gpn
instances ag4m502deduir5s86g1i --profile=gpn
