#!/bin/sh

echo ">>> Launch Elastic Search-Nodes"
echo "========================================================================="

export RESOLVED_HOST_NAME=$(deploy_unit_resolver --print_hostname)
export DATACENTER=$(deploy_unit_resolver --print_cluster)

export ROLES=[]
export CPU_COUNT=16
export ES_JAVA_OPTS="-Xms16g -Xmx16g  -Des.transport.cname_in_publish_address=true -Dlog4j2.formatMsgNoLookups=true -Dlog4j.formatMsgNoLookups=true"

/usr/share/elasticsearch/bin/elasticsearch  -Ecluster.remote.cls.seeds=$(deploy_unit_resolver --deploy_unit=DataNodes,MasterNodes --clusters=man,sas,vla,iva,myt --print_fqdns --add_port 8991)
