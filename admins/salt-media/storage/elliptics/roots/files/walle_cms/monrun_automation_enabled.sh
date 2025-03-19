#!/bin/bash

code=0
declare -a failed_projects
token=$(cat /etc/yandex/mds_cms/walle_token)

for project in avatars resizer mds-proxy mds-storage; do
    msg="$(curl -s -H 'Authorization: Bearer '$token -H 'Content-Type: application/json' 'https://api.wall-e.yandex-team.ru/v1/projects/'$project'?fields=healing_automation.enabled')"
    if [ "$msg" != '{"healing_automation": {"enabled": true}}' ]; then
        code=2;
        failed_projects+=($project)
    fi
done

if [ $code == 0 ]; then
   echo "0; OK"
else
   echo "2; Automation disabled in " ${failed_projects[*]}
fi
