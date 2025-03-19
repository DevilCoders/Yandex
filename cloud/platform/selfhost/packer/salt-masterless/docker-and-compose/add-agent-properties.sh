#!/usr/bin/env bash

propFile="/data/teamcity_agent/conf/buildAgent.properties"

echo "# Custom properties for agent by 'add-agent-properties.sh'" >> $propFile
echo "" >> $propFile
echo "yandex.cloud.agent_image_datetime=${AGENT_IMAGE_DATETIME}" >> $propFile
echo "yandex.cloud.endpoint=${YC_ENDPOINT}"          >> $propFile
echo "yandex.cloud.instance_id=${YC_INSTANCE_ID}"    >> $propFile
echo "yandex.network.project_id=${PROJECT_ID}"       >> $propFile
authToken=$(echo -n "${TEAMCITY_SEED}_$(hostname)" | md5sum | awk '{print $1}')
echo "# pregenerated authorization token for Teamcity Server (https://nda.ya.ru/t/TrQpEwfK3VwA6A)"  >> $propFile
echo "authorizationToken=${authToken}"            >> $propFile
echo "# END of Custom properties for agent by 'add-agent-properties.sh'" >> $propFile
