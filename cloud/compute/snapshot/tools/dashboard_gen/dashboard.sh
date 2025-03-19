#!/bin/bash

if [ -z "$ACTION" ]
then
	ACTION=local
fi
if [ -z "$GRAFANA_OAUTH_TOKEN" ]
then
	echo OAUTH token not specified, set GRAFANA_OAUTH_TOKEN env var
	exit 1
fi

# our config file
source specs.env

for i in "${!DASHBOARD_ID[@]}"; do
    # Original dashboard
    sed -e "s/<DASHBOARD_ID>/${DASHBOARD_ID[$i]}/" \
        -e "s/<STAND_ID>/${STAND_ID[$i]}/"   \
        -e "s/<DATASOURCE_ID>/${DATASOURCE_ID[$i]}/" \
        -e "s/<DASHBOARD_TITLE>/${DASHBOARD_TITLE[$i]}/" \
        -e "s/<TEXT_LINK_OTHER_BOARD>/Дашборд с информацией по хостам/" \
        -e "s/<SECONDARY_DASHBOARD_ID>/${SECONDARY_DASHBOARD_ID[$i]}/" \
        -e "s/<HOST_VAR>/zone/" \
        -e "s/<ZONE_VALUES>/${ZONE_VALUES[$i]}/" \
        -e "s/<ZONE_TITLES>/${ZONE_TITLES[$i]}/" \
        -e "s/<HIDDEN_HOST_VAR>/true/" \
        -e "s/<SOLOMON_PROJECT>/${SOLOMON_PROJECT[$i]}/" \
         template.yaml > "${DASHBOARD_ID[$i]}-tmp.yaml"
    docker run --rm   "--mount=type=bind,source=$PWD,target=/arc" -e GRAFANA_OAUTH_TOKEN="$GRAFANA_OAUTH_TOKEN" -e DASHBOARD_ACTION="$ACTION" -e DASHBOARD_SPEC="/arc/"${DASHBOARD_ID[$i]}-tmp.yaml"" registry.yandex.net/cloud/platform/dashboard:latest >&2
    echo "https://grafana.yandex-team.ru/d/${DASHBOARD_ID[$i]} - ${DASHBOARD_TITLE[$i]}"

    # Secondary dashoard
    sed -e "s/<DASHBOARD_ID>/${SECONDARY_DASHBOARD_ID[$i]}/" \
        -e "s/<STAND_ID>/${STAND_ID[$i]}/"   \
        -e "s/<DATASOURCE_ID>/${DATASOURCE_ID[$i]}/" \
        -e "s/<DASHBOARD_TITLE>/${DASHBOARD_TITLE[$i]} Hosts/" \
        -e "s/<TEXT_LINK_OTHER_BOARD>/Дашборд c информацией по стендам/" \
        -e "s/<SECONDARY_DASHBOARD_ID>/${DASHBOARD_ID[$i]}/" \
        -e "s/<HOST_VAR>/host/" \
        -e "s/<ZONE_VALUES>/${ZONE_VALUES[$i]}/" \
        -e "s/<ZONE_TITLES>/${ZONE_TITLES[$i]}/" \
        -e "s/<HIDDEN_HOST_VAR>/false/" \
        -e "s/<SOLOMON_PROJECT>/${SOLOMON_PROJECT[$i]}/" \
         template.yaml > "${SECONDARY_DASHBOARD_ID[$i]}-tmp.yaml"
    docker run --rm "--mount=type=bind,source=$PWD,target=/arc" -e GRAFANA_OAUTH_TOKEN="$GRAFANA_OAUTH_TOKEN" -e DASHBOARD_ACTION="$ACTION" -e DASHBOARD_SPEC="/arc/"${SECONDARY_DASHBOARD_ID[$i]}-tmp.yaml"" registry.yandex.net/cloud/platform/dashboard:latest >&2
    echo "https://grafana.yandex-team.ru/d/${SECONDARY_DASHBOARD_ID[$i]} - ${DASHBOARD_TITLE[$i]} - hosts"
    if [ "$ACTION" != "local" ]
    then
        rm ./*tmp.json
        rm ./*tmp.yaml
    fi

done
