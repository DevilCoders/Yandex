#!/bin/bash

lastalive_file='{{ pillar['oct']['database']['cassandra_lastalive_file'] }}'
no_rejoin_after='{{ pillar['oct']['database']['cassandra_dont_rejoin_after'] }}' # seconds of downtime

if [ ! -f "$lastalive_file" ]; then
    echo "No lastalive timestamp file found."
    echo "Assuming everything is OK."

    echo "Creating $lastalive_file with current timestamp."
    date +"%s" > $lastalive_file

    echo "Proceeding to start Cassandra"
    exit 0
fi


now=$(date +"%s")
lastalive_time=$(cat $lastalive_file)
lastalive_time_int=${lastalive_time%.*}

if (( lastalive_time_int + no_rejoin_after < now )); then
   echo "Cassandra has been in a failed state for more than $no_rejoin_after seconds (since $lastalive_time_int)"
   echo "That's too long to rejoin cluster. You have to clean data."
   echo "See wiki for details: https://wiki.yandex-team.ru/cloud/devel/sdn/tooling/#troubleshoot-cassandra"
   echo ""
   echo "Delete $lastalive_file if you want to force starting Cassandra"
   exit 1
fi

echo "Cassandra have been in a failed state for less than $no_rejoin_after seconds"
echo "Proceeding to start Cassandra"

exit 0
