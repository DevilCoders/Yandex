#!/bin/bash

if [ -z "$INSTALLATION" ] && [ -f /installation ]
then
    INSTALLATION=`cat /installation`
fi

if [ -L /config ]
then
    rm /config
fi

if [ ! -e /config ]
then
    CONFIG_SOURCE="/config-sources/${INSTALLATION:-default}"
    if [ ! -e "$CONFIG_SOURCE" ]
    then
        CONFIG_SOURCE="/config-sources/default"
    fi
    ln -s "$CONFIG_SOURCE" /config

    echo /config was not mounted and selected as "$CONFIG_SOURCE"
fi

case "$1" in
    jaeger) jaeger-agent --config-file /config/jaeger.yaml ;;
    unified-agent) unified_agent --config /etc/yandex/unified_agent/config.yml ;;
    tvm) TVMTOOL_LOCAL_AUTHTOKEN=`cat /config/tvm/api_auth` tvmtool -e -p 18080 -c /config/tvm/config.json ;;
    *) "$@" ;;
esac
