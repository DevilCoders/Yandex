#!/usr/bin/env bash

if [[ -z "$YC_OAUTH" ]]; then
    YC_OAUTH="$YC_TOKEN"
fi
if [[ -z "$YC_OAUTH" ]]; then
    echo "YC_OAUTH env variable missing"
    exit 1
fi

if [[ -z "$YT_OAUTH" ]]; then
    YT_OAUTH="$YT_TOKEN"
fi
if [[ -z "$YT_OAUTH" ]]; then
    echo "YT_OAUTH env variable missing"
    exit 1
fi

CONFIG='
{
    "mqtt": {
        "vla": [
            {
                "target": "yandex_compute_instance.mqtt-instance-group[0]",
                "hostname": "2a02:6b8:c0e:501:0:f806:0:16f"
            }
        ],
        "sas": [
            {
                "target": "yandex_compute_instance.mqtt-instance-group[1]",
                "hostname": "2a02:6b8:c02:901:0:f806:0:403"
            }
        ]
    },
    "devices": {
        "vla": [
            {
                "target": "module.devices-instance-group.yandex_compute_instance.node[0]",
                "hostname": "2a02:6b8:c0e:501:0:f806:0:1a7"
            }
        ],
        "sas": [
            {
                "target": "module.devices-instance-group.yandex_compute_instance.node[1]",
                "hostname": "2a02:6b8:c02:901:0:f806:0:36f"
            }
        ]
    },
    "events": {
        "vla": [
            {
                "target": "module.events-instance-group.yandex_compute_instance.node[0]",
                "hostname": "2a02:6b8:c0e:501:0:f806:0:f1"
            }
        ],
        "sas": [
            {
                "target": "module.events-instance-group.yandex_compute_instance.node[1]",
                "hostname": "2a02:6b8:c02:901:0:f806:0:28"
            }
        ]
    }
}'

exit_if_error() {
    if [[ "$1" -ne "0" ]]; then
        echo "Error occured" 1>&2
        exit "$1"
    fi
}

log_and_eval() {
    CMD=$1
    echo ${CMD} \
        | sed -e 's/yc_token=\([^ ]*\)/yc_token=\$\{YC_OAUTH\}/' \
        | sed -e 's/yandex_token=\([^ ]*\)/yandex_token=\$\{YT_OAUTH\}/'
    eval ${CMD}
}

get_instance_field() {
    if [[ "$#" -ne "4" ]]; then
        echo "You should provide SERVICE LOCATION INDEX arguments" 1>&2
        exit 1
    fi

    SERVICE="$1"
    LOCATION="$2"
    INDEX="$3"
    FIELD="$4"

    if [[ "$SERVICE" == "all" ]]; then
        SELECTOR1="[]"
    else
        SELECTOR1="[\"$SERVICE\"]"
    fi
    if [[ "$LOCATION" == "all" ]]; then
        SELECTOR2="[]"
    else
        SELECTOR2="[\"$LOCATION\"]"
    fi
    if [[ "$INDEX" == "all" ]]; then
        SELECTOR3="[]"
    else
        SELECTOR3="[$INDEX]"
    fi
#    echo ".${SELECTOR1}?${SELECTOR2}?${SELECTOR3}?[\"$FIELD\"]?" 1>&2
    TARGETS=`echo "$CONFIG" | jq -r ".${SELECTOR1}?${SELECTOR2}?${SELECTOR3}?[\"$FIELD\"]?" | sed -e '/^null$/d'`
    if [[ -z "$TARGETS" ]]; then
        echo "Nothing matches instance #$INDEX for service '$SERVICE' in '$LOCATION' field '$FIELD'" 1>&2
        exit 1
    fi
    echo "$TARGETS"
}

get_instance_count() {
    SERVICE="$1"
    LOCATION="$2"

    RESULT=`get_instance_field "$SERVICE" "$LOCATION" all hostname`
    exit_if_error "$?"
    echo `echo "$RESULT" | wc -l`
}

get_locations() {
    SERVICE="$1"
    LOCATIONS=`echo "$CONFIG" | jq -r ".[\"$SERVICE\"] | keys[]" | sed -e '/^null$/d'`
    if [[ -z "$LOCATIONS" ]]; then
        echo "Not found locations for service $SERVICE" 1>&2
        exit 1
    fi
    echo "$LOCATIONS"
}
