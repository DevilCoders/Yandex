#!/bin/bash

max_time=${1:-15}

stderr_file=$(mktemp)
stdout_file=$(mktemp)

sudo timeout -s SIGKILL "${max_time}" karl-cli control red_alert status --tls 2>"$stderr_file" > "$stdout_file"
code=$?

if [ $code -eq 0 ]
then
    mode=$(grep -Po "KARL_MODE_\w+" "$stdout_file")
    if [ "$mode" == "KARL_MODE_OK" ]
    then
        echo '0; Ok'
    else
        echo "2; Current mode isn't 'OK': $mode"
    fi
else
    if [ ${code} -eq 124 ]
    then
        echo "2; 'karl-cli control red_alert status' exited with code '${code}': timed out adter ${max_time} seconds"
    else
        echo "2; 'karl-cli control red_alert status' exited with code '${code}': $(head -n1 "$stderr_file")"
    fi
fi

rm "$stderr_file"
rm "$stdout_file"
