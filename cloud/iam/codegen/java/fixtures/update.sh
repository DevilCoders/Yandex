#!/bin/bash
set -e
SRC=https://teamcity.yandex-team.ru/repository/download/Cloud_CloudGo_IamCompileRoleFixtures/.lastFinished/fixture_permissions.yaml

if [[ -z ${OAUTH_TOKEN} ]]; then
  echo "Usage: OAUTH_TOKEN=... $0"
  echo "Please visit https://oauth.yandex-team.ru/authorize?response_type=token&client_id=7feab4f3dd964c95b57ba7b6fa085cc5 to get your token"
  exit 0
fi

if [[ $(arc status --json --branch | jq -r '.branch_info.local.name') == "trunk" ]]; then
  echo "Current branch is trunk, please checkout another branch"
  exit 0;
fi

wget --quiet --header "Authorization: OAuth ${OAUTH_TOKEN}" "${SRC}" -O src/main/resources/yandex/cloud/iam/fixture_permissions.yaml
sed -i.bak "s/^#JAVA_SRCS/JAVA_SRCS/g; s/EXTERNAL_JAR/#EXTERNAL_JAR/g" ya.make &&  rm ya.make.bak
ya make -r --checkout -D YMAKE_JAVA_MODULES=false
SANDBOX_RESOURCE=$(ya upload --json-output --ttl=inf -T=JAVA_LIBRARY -a=linux ./yandex-cloud-iam-fixtures.jar | jq .resource_id)
sed -i.bak "s/#EXTERNAL_JAR(sbr:.*)/EXTERNAL_JAR(sbr:${SANDBOX_RESOURCE})/g; s/^JAVA_SRCS/#JAVA_SRCS/g; s/FROM_SANDBOX(FILE .* RENAME/FROM_SANDBOX(FILE ${SANDBOX_RESOURCE} RENAME/g" ya.make
rm -f ya.make.bak

COMMIT_MSG="Update cloud-java constants to ${SANDBOX_RESOURCE}"
arc commit -m "$COMMIT_MSG" ya.make
PR_ID=$(ya pr c --push --publish --auto -m "$COMMIT_MSG" --json | jq -r '.id')

echo "PR $PR_ID created, waiting for merge. Please ship or self-ship."
arc pr view --id "$PR_ID" 2>/dev/null # --shipit flag is deprecated but --no-code-review is not released yet, so this step is manual for now

while :; do
  PR_STATUS=$(arc pr st "$PR_ID" --json | jq -r '.status')
  if [ "$PR_STATUS" == "merged" ] ; then
    break
  else
    echo "PR $PR_ID status is $PR_STATUS"
    sleep 30
  fi
done

arc checkout trunk
arc pull
../maven_deploy.sh