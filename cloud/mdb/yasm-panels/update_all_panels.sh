#!/bin/sh

set -x

curl --data-binary @templates/pgaas_containers.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=pgaas_container_metrics' >/dev/null
curl --data-binary @templates/pgaas_proxy.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=pgaas_proxy' >/dev/null
curl --data-binary @templates/dbaas_postgres.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=dbaas_postgres_metrics' >/dev/null
curl --data-binary @templates/dbaas_mysql.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=dbaas_mysql_metrics' >/dev/null
curl --data-binary @templates/dbaas_clickhouse.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=dbaas_clickhouse_metrics' >/dev/null
curl --data-binary @templates/dbaas_mongodb.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=dbaas_mongodb_metrics' >/dev/null
curl --data-binary @templates/dbaas_redis.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=dbaas_redis_metrics' >/dev/null
curl --data-binary @templates/pgaas.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=pgaas_metrics' >/dev/null
curl --data-binary @templates/s3.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=mdb_s3' >/dev/null
curl --data-binary @templates/s3_shard.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=mdb_s3db' >/dev/null
curl --data-binary @templates/s3meta_shard.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=mdb_s3meta' >/dev/null
curl --data-binary @templates/s3proxy.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=mdb_s3proxy' >/dev/null
curl --data-binary @templates/s3db.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=mail_s3db_counters' >/dev/null
curl --data-binary @templates/s3meta.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=mail_s3meta_counters' >/dev/null
curl --data-binary @templates/dom0.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=mdbdom0' >/dev/null
curl --data-binary @templates/dbaas_resources.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=dbaas_resources' >/dev/null
curl --data-binary @templates/dbaas_internal_api.json 'https://yasm.yandex-team.ru/srvambry/templates/update/content?key=dbaas_internal_api' >/dev/null
