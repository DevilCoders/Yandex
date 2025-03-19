#!/bin/bash

if [ $# -lt 1 ]
then
    echo "usage: $0 <pre_restart|post_restart>"
    exit 1
fi

script_type=$1

if [[ "$script_type" != "pre_restart" ]] && [[ "$script_type" != "post_restart" ]]
then
    echo "Script type should be one of: pre_restart, post_restart"
    exit 1
fi

for container in $(portoctl list -r1 -t | grep '\.')
do
    if portoctl shell "$container" stat "/usr/local/yandex/$script_type.sh" >/dev/null 2>/dev/null
    then
        echo "Starting $script_type for container $container"
        portoctl shell "$container" "/usr/local/yandex/$script_type.sh"
        if [ "$?" != "0" ]
        then
            echo "$script_type in container $container failed with exit code $?"
            exit 1
        fi
    else
        echo "Skipping $script_type for container $container (/usr/local/yandex/$script_type.sh not found)"
    fi
done
