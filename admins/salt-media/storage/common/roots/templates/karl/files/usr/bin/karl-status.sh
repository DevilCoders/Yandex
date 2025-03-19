#!/bin/bash

max_time=${1:-15}

stderr_file=$(mktemp)
sudo timeout -s SIGKILL "${max_time}" karl-cli ping --tls 2>"$stderr_file" >/dev/null
code=$?

if [ $code -eq 0 ]
then
	echo '0; Ok'
else
    if [ ${code} -eq 124 ]
    then
        echo "2; 'karl-cli ping' exited with code '${code}': timed out adter ${max_time} seconds"
    else
        echo "2; 'karl-cli ping' exited with code '${code}': $(head -n1 "$stderr_file")"
    fi
fi

rm "$stderr_file"
