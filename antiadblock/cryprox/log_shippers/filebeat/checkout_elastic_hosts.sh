#!/bin/sh

filename='elastic_hosts.txt'

new_elastic_hosts=$(deploy_unit_resolver --deploy_unit=ElasticClusters --deploy_stage=antiadb-elasticsearch --clusters=man,sas,vla --print_fqdns --add_port 8890)

if [ -f "$filename" ]
then
    elastic_hosts=$(<$filename)
    if [ "$elastic_hosts" != "$new_elastic_hosts" ]
    then
        pkill filebeat
    fi
else
    pkill filebeat
fi

echo $new_elastic_hosts > $filename

