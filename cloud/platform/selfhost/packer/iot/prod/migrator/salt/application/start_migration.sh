#!/usr/bin/env bash

init_docker.sh || exit 1
docker run -it --net host\
    -v '/var/log/yc-iot/migrator:/var/log/yc-iot/migrator'\
    -v '/etc/yc-iot/migrator:/etc/yc-iot/migrator'\
    -e 'JAVA_TOOL_OPTIONS=-Xms512m -Xmx512m -XX:ReservedCodeCacheSize=64m -XX:+UseCompressedOops -Djava.net.preferIPv6Addresses=true -Dfile.encoding=UTF-8 -Xlog:gc*,safepoint:/var/log/yc-iot/migrator/gc.log:time,uptime:filecount=32,filesize=32M -Dlog4j.configurationFile=/etc/yc-iot/migrator/log4j2.yaml'\
    -e 'IOT_ID_PREFIX=are' \
    -e 'IOT_KIKIMR_ENDPOINT=ydb-iotdevices.cloud.yandex.net:2135' \
    -e 'IOT_KIKIMR_TABLESPACE=/global/iotdevices' \
    -e 'IOT_KIKIMR_DATABASE=/global/iotdevices' \
    -e 'IOT_FOLDER_ID=b1gq57e4mu8e8nhimac0' \
    -e 'IOT_RESOURCE_MANAGER_ENDPOINT=api-adapter.private-api.ycp.cloud.yandex.net:443' \
    -e 'IOT_LOG_GROUP_ENDPOINT=log-groups.private-api.ycp.cloud.yandex.net:443' \
    'registry.yandex.net/cloud/devices-migrator:{{ migrator_application_version }}' "$@" || exit 1
