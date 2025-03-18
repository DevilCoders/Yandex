#!/bin/sh

echo ">>> Launch Elastic Data-Nodes"
echo "========================================================================="

export RESOLVED_HOST_NAME=$(deploy_unit_resolver --print_hostname)
export DATACENTER=$(deploy_unit_resolver --print_cluster)

export ROLES=[data]
export CPU_COUNT=6
export ES_JAVA_OPTS="-Xms16g -Xmx16g -Des.transport.cname_in_publish_address=true -Dlog4j2.formatMsgNoLookups=true -Dlog4j.formatMsgNoLookups=true"

/usr/share/elasticsearch/bin/elasticsearch -Ecluster.initial_master_nodes=$(deploy_unit_resolver --deploy_units=MasterNodes --print_fqdns --clusters=vla,sas,iva,myt,man)
