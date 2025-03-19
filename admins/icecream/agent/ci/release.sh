#!/usr/bin/env bash

# Gen new version, build package and upload it to dist
set -exu -o pipefail

PROJECT_DIR=`pwd`
OPTIONS=""
RELEASE_BRANCH="release"
OPTIONS+="-R -D unstable"
OPTIONS+=" --debian-branch=$RELEASE_BRANCH"
OPTIONS+=" --spawn-editor=snapshot"


cd ${PROJECT_DIR}
git pull
git checkout -B ${RELEASE_BRANCH}


if ! [ -e "$PROJECT_DIR/debian/changelog" ]; then
    echo "debian/changelog not found in $PROJECT_DIR" >&2
    exit 2
fi


CUR_VERSION=$(dpkg-parsechangelog --show-field Version)
NEW_VERSION=$(echo ${CUR_VERSION} | awk 'BEGIN {FS=".";OFS="."} {$NF=$NF+1;print $0}')


gbp dch -c -N ${NEW_VERSION} ${OPTIONS}
gbp buildpackage --git-tag --git-ignore-new --git-no-create-orig
git push --set-upstream origin ${RELEASE_BRANCH}
git push --tags

debrelease --nomail

GITHUB_USER="teamcity:Cleph8OvOwd"
REQUEST_TITLE="Release of ${NEW_VERSION}"
REQUEST_BODY="$(ci/gh_changelog.py | awk 1 ORS='\\n' | sed 's/"/\\"/g')"

GITHUB_DATA=$(cat<<END
{
  "title": "${REQUEST_TITLE}",
  "head": "${RELEASE_BRANCH}",
  "base": "master",
  "body": "${REQUEST_BODY}"
}
END
)

curl -u "$GITHUB_USER" -d "$GITHUB_DATA" https://api.github.yandex-team.ru/repos/yandex-icecream/agent/pulls
