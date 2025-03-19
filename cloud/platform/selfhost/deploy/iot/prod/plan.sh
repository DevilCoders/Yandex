#!/usr/bin/env bash

which yq >/dev/null
if [ $? -ne 0 ]; then
  echo "yq is required https://github.com/kislyuk/yq
  Linux:
  sudo pip install yq
  Mac:
  brew install python-yq"
  exit 1
fi

cd "$(dirname "${BASH_SOURCE[0]}")" || exit 1
source common.sh

####### DIFF
spec_for_diff() {
  DEL_KEYS=".created_at,
  .load_balancer_state,
  .managed_instances_state,
  .status,
  .platform_l7_load_balancer_spec,
  .load_balancer_state,
  .instance_template.hostname,
  .instance_template.name,
  .folder_id,
  .id"

  SED_UNWRAP_VALUES='s/"\([^"]*\)"$/\1/'
  SED_REMOVE_EOL_COMMA='s/,$//'
  yq -S "del(${DEL_KEYS})" "${1}" | sed -e "${SED_REMOVE_EOL_COMMA};${SED_UNWRAP_VALUES}"
}

SPEC_CURRENT=${TMP_DIR}/current.yaml
yc compute instance-group get --id ${INSTANCE_GROUP_ID} --full --format yaml --endpoint ${API_ENDPOINT} > ${SPEC_CURRENT}

SPEC_RENDERED_FOR_DIFF=${TMP_DIR}/rendered-for-diff.txt
SPEC_CURRENT_FOR_DIFF=${TMP_DIR}/current-for-diff.txt
spec_for_diff "${SPEC_RENDERED}" >${SPEC_RENDERED_FOR_DIFF}
spec_for_diff "${SPEC_CURRENT}" >${SPEC_CURRENT_FOR_DIFF}

colordiff -Nua --label 'current spec' "${SPEC_CURRENT_FOR_DIFF}" --label 'new spec' "${SPEC_RENDERED_FOR_DIFF}"
if [[ $? -lt 2 ]]; then
  exit 0
fi
echo "colordiff is not installed, will try diff --color"
diff -Nua --color --label 'current spec' "${SPEC_CURRENT_FOR_DIFF}" --label 'new spec' "${SPEC_RENDERED_FOR_DIFF}"
if [[ $? -lt 2 ]]; then
  exit 0
fi
echo "falling back to uncolored diff"
diff -Nua --label 'current spec' "${SPEC_CURRENT_FOR_DIFF}" --label 'new spec' "${SPEC_RENDERED_FOR_DIFF}"
