#!/bin/bash
#set -x #echo on

function current_day() {
    echo "$(TZ=UTC date +"%Y-%m-%d")"
}

if [[ -z "$DAY" ]]; then
    DAY=$(current_day)
    if [[ ! "$SILENT" == "true" ]]; then
        (>&2 echo "Use current day - $DAY")
    fi
elif [[ ! "$DAY" =~ ^[1-2][0-9]{3}-[0-9]{2}-[0-9]{2}$ ]]; then
    INVALID_DAY=${DAY}
    DAY=$(current_day)
    if [[ ! "$SILENT" == "true" ]]; then
        (>&2 echo "Invalid day ${INVALID_DAY}, use current day - $DAY")
    fi
else
    if [[ ! "$SILENT" == "true" ]]; then
        (>&2 echo "Use day - $DAY")
    fi
fi
echo ${DAY}
