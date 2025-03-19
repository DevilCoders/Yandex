#!/usr/bin/env bash
THIS_SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

[ "$IAM_METADATA_ENV_VARS_AVAILABLE" != "yes" ] && echo "please init settings and variables by running \`source ./env.sh\`" && exit 1
TICKET=${TICKET:?"TICKET env variable is required"}

set -e
set -o pipefail

ARC_IAM_SCRIPTS="${THIS_SCRIPT_DIR}/.."
PROFILE=${1:?}
LOCAL_DIR="./current"

logfile=$(date +".release-%Y%m%d%H%M%S.log")
ycp --profile $PROFILE iam inner metadata update \
  --from-file ${LOCAL_DIR}/${PROFILE}_dump.yaml \
  --export-file ${LOCAL_DIR}/${PROFILE}_export.yaml \
  --no-color=false | tee -a $logfile # retain colors for stdout but uncolor the file produced by tee

# TODO: if you say "no" to any prompt, ycp exit code will be 0, need to fix that on ycp side

comment_log=$(cat $logfile | sed 's/\x1B\[[0-9;]\{1,\}[A-Za-z]//g') # uncolor
comment_log=$(echo "$comment_log" | sed -E 's/( +)([-+])/\2\1/') # move diff signs - and + to the beginning in order to conform the markdown diff format
comment=$(printf 'Update IAM metadata on %s:
<{OK
```diff
%s
```
}>' $PROFILE "$comment_log")
${ARC_IAM_SCRIPTS}/add_startrek_comment.sh $TICKET "$comment"
