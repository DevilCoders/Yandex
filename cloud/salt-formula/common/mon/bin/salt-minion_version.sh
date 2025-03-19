#!/usr/bin/env bash
demand_saltminion="install ok installed 2017.7.2-yandex1"
curr_saltminion=$(dpkg-query -W -f='${Status} ${Version}\n' salt-minion)

if [ "${demand_saltminion}" = "${curr_saltminion}" ]; then
    echo "PASSIVE-CHECK:minion_version;0;OK"
else
    echo "PASSIVE-CHECK:minion_version;1;${curr_saltminion}. Needed ${demand_saltminion}"
fi
