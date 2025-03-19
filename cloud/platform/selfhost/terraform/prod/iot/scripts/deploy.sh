#!/usr/bin/env bash

source "${BASH_SOURCE%/*}/common.sh"

if [[ "$#" -lt "1" || "$#" -gt "2" ]]; then
    echo "Usage: deploy.sh SERVICE [LOCATION]" 1>&2
    exit 10
fi

SERVICE=$1
LOCATION=$2

if [[ -z "$LOCATION" || "$LOCATION" == "all" ]]; then
    LOCATIONS=`get_locations "$SERVICE"`
    exit_if_error "$?"
else
    LOCATIONS="$LOCATION"
fi

LOCATIONS_LINE=`echo "$LOCATIONS" | sed -ze 's/\n/ /g'`
echo "Going to deploy service '$SERVICE' to locations: $LOCATIONS_LINE"

run_healthcheck() {
    SERVICE="$1"
    LOCATION="$2"
    INDEX="$3"
    case "$SERVICE" in
        mqtt)
            ${BASH_SOURCE%/*}/mqtt_healthcheck.sh "$LOCATION" "$INDEX"
        ;;
        devices)
            ${BASH_SOURCE%/*}/devices_healthcheck.sh "$LOCATION" "$INDEX"
        ;;
        events)
            ${BASH_SOURCE%/*}/events_healthcheck.sh "$LOCATION" "$INDEX"
        ;;
        *)
            echo "Healthcheck for $SERVICE not implemented" 1>&2
            exit 10
        ;;
    esac
}

for LOCATION in $LOCATIONS
do
    if [[ "$LOCATION" == "all" ]]; then
        echo "Unexpected location 'all'"
        exit 1
    fi
    read -p "Are you sure you want to deploy '$SERVICE' to '$LOCATION'? <y/N> " prompt
    if [[ $prompt != "y" && $prompt != "Y" && $prompt != "yes" && $prompt != "Yes" ]]
    then
      exit 0
    fi

    COUNT=`get_instance_count "$SERVICE" "$LOCATION"`
    for (( INDEX=0; INDEX<COUNT; INDEX++ ))
    do
	    ${BASH_SOURCE%/*}/apply.sh "$SERVICE" "$LOCATION" "$INDEX"
	    exit_if_error "$?"
        ERROR_STATUS="0"
        while [[ "$ERROR_STATUS" == "0" ]]
        do
            echo "Waiting host to go down"
            run_healthcheck "$SERVICE" "$LOCATION" "$INDEX"
            ERROR_STATUS="$?"
            sleep 1
        done
        while [[ "$ERROR_STATUS" == "1" ]]
        do
            sleep 2
            echo "Waiting host to go up"
            run_healthcheck "$SERVICE" "$LOCATION" "$INDEX"
            ERROR_STATUS="$?"
        done
        if [[ "$ERROR_STATUS" -ne "0" ]]; then
            exit "$ERROR_STATUS"
        fi
    done
done

echo "DEPLOY OK"
