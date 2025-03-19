#!/bin/bash

for container in $(portoctl list -r1 -t | grep '\.')
do
    if portoctl shell "$container" stat "/bin/systemctl" >/dev/null 2>/dev/null
    then
        echo "Fixing systemd for container $container"
        portoctl shell "$container" /bin/systemctl daemon-reexec
        if [ "$?" != "0" ]
        then
            echo "Failed with exit code $?"
            exit 1
        fi
    else
        echo "Skipping systemd fix container $container (/bin/systemctl not found)"
    fi
done
