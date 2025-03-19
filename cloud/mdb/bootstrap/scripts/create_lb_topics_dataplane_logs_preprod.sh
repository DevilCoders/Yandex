set -x
# Topic configuration for compute preprod
#
# You are also required to set YC_TOKEN for auth to pass with LB installation.
# As of 19.01.21 federated accounts are NOT supported in YC LB.
# See https://cloud.yandex.ru/docs/iam/concepts/authorization/oauth-token for instruction on how to obtain a passport-based one.
# YC_TOKEN="<passport oauth token>"
LB_UTIL='ya tool logbroker -s yc-logbroker-preprod'

# Logbroker base account id (everywhere else that would be "cloud id" with regards to hierarchy level)
lb_account_id='aoe9shbqc2v314v7fp3d'
# Datatransfer service account ID
dt_account_id='bfbgnr9a209qmtjepb55@as'
# Producer -- dataplane VMs shipping logs
producer_sa='yc.mdb.logs_dataplane_producer@as'
# Virtual LB entity performing the read
consumer='logsdb-consumer'
# How many parts each topic will be divided on
number_of_partitions=3

test -z "${YC_TOKEN}" && { echo "YC_TOKEN env var is required. Look inside for instructions"; exit 1; }

. ./logbroker.include.sh

create_consumer "/${lb_account_id}/dataplane/${consumer}"

for db in kafka elasticsearch kibana clickhouse \
    postgresql odyssey pgbouncer \
    mongodb mssql mysql-audit \
    mysql-error mysql-general mysql-slow \
    redis security greenplum greenplum-odyssey
do
    topic="/${lb_account_id}/dataplane/${db}-logs"

    create_topic ${topic} ${number_of_partitions}
    allow_consumer_read_topic ${consumer} ${topic}
    allow_producer_write_topic ${producer_sa} ${topic}
    allow_datatransfer_read_topic ${dt_account_id} ${topic}
    allow_datatransfer_read_topic_as_consumer ${dt_account_id} ${topic} ${consumer}
done
