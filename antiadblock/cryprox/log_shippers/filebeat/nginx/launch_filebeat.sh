#!/bin/sh

export ELASTIC_HOSTS=$(deploy_unit_resolver --deploy_stage=antiadb-elasticsearch --deploy_units=DataNodes --clusters=man,sas,vla --print_fqdns --add_port 8890)

export FILEBEAT_TAGS=['nginx','yp']

filebeat -path.config /log_shippers/filebeat/nginx -path.data /logs/filebeat-nginx -strict.perms=false -v
