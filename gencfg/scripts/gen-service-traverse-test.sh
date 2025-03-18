#!/usr/bin/env bash

main() {
    uuid=$1
    instance_type=$2
    db_type="tags"

    valid_instance_type=0
    if [ "$instance_type" == "api" ]; then
        valid_instance_type=1
        port=10033
    fi
    if [ "$instance_type" == "wbe" ]; then
        valid_instance_type=1
        port=10036
    fi
    if [ "$valid_instance_type" -eq 0 ] ; then
        echo "Unsupported instance type \"$instance_type\""
        return 1
    fi

    echo "Starting service on port ${port}..."
    "${instance_type}/main.py" -p "$port" --db "$db_type" --no-auth --no-commit --uuid "${uuid}" >"$execute_log" 2>&1 &

    sleep 5

    echo "Testing service..."
    if ! curl -f "localhost:${port}/check" >/dev/null 2>&1 ; then
        echo "Failed to get /check result"
        return 1
    fi

    # TODO: print log in output

    echo "Running web_utils/dolbilo.py..."
    "web_utils/dolbilo.py" -p "${port}" -b "${instance_type}" -d 10 --tags-filter "latest" --continue-on-error
    dresult="$?"

    echo "Sending die request..."
    curl -X POST "localhost:${port}/setup?action=die" >/dev/null 2>&1
    sleep 3

    if [ ${dresult} -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

_uuid=$(cat /proc/sys/kernel/random/uuid)
main "$_uuid" $@
result="$?"

pid=`ps aux | grep "${_uuid}" | grep -v grep | awk '{ print \$2 }'`
if [ "$pid" != "" ]; then
    echo "Killing service..."
    kill -9 "${pid}"
fi

if [ "$result" -eq 0 ]; then
    # works well when running by "source" command
    echo "Success"
    return 0 2> /dev/null || exit 0
else
    echo "Failed"
    # works well when running by "source" command
    return 1 2> /dev/null || exit 1
fi
