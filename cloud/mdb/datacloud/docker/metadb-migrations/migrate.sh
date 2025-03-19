#!/bin/bash
set -e

HOSTSPEC=$(python3 -c "hosts = '$PGHOST'; port = '$PGPORT'; \
  print(','.join('{}:{}'.format(host, port) for host in hosts.split(',')))")

DSN="postgresql://$PGUSER:$PGPASSWORD@$HOSTSPEC/$PGDATABASE?sslmode=verify-full&sslrootcert=$PGSSLROOTCERT&target_session_attrs=read-write&connect_timeout=5"

CREATE_USERS=${CREATE_USERS-yes}
if [[ "$CREATE_USERS" == "yes" ]]; then
  create_users -g /opt/datacloud/dbaas_metadb/head/grants -a meta -c "$DSN"
fi;

mkdir -p /opt/metadb/resources/
PLPYTHON_AVAILABLE=${PLPYTHON_AVAILABLE-false}
if [[ "$PLPYTHON_AVAILABLE" == "false" ]]; then
  # TODO: remove next line after ORION-95 (plpython)
  cp /opt/datacloud/resources/code/05_combine_dict.sql /opt/datacloud/dbaas_metadb/migrations/V0353__combine_dict.sql
fi;

# Two 'moments' bring that sed:
# 1. We use dbname=metadb instead of dbaas_metadb in DC (I think that problem near `create_users` and Secret Manager secret IDs)
# 2. In YC control plane databases users are creating without `CONNECT` privilege.
#    So we start to add `GRANT CONNECT ON DATABASE dbaas_metadb TO ...` to our grants. https://t.me/c/1229998180/17638
sed -i "s/GRANT CONNECT ON DATABASE dbaas_metadb/GRANT CONNECT ON DATABASE $PGDATABASE/" /opt/datacloud/dbaas_metadb/migrations/V0355__grants.sql
sed -i "s/GRANT CONNECT ON DATABASE dbaas_metadb/GRANT CONNECT ON DATABASE $PGDATABASE/" /opt/datacloud/dbaas_metadb/migrations/V0372__Add_grants_for_billing_bookkeeper.sql

if [[ "$EXTENSIONS_ALREADY_CREATED" == "false" ]]; then
  if [[ "$PLPYTHON_AVAILABLE" == "false" ]]; then
    sed -i '/CREATE EXTENSION IF NOT EXISTS plpythonu;/d' /opt/datacloud/dbaas_metadb/migrations/V0270__Trigger_constraint_versions_updatable_to.sql
    echo '' > /opt/datacloud/dbaas_metadb/migrations/V0346__plpython3.sql
  fi;
else
  sed -i '/CREATE EXTENSION/d' /opt/datacloud/dbaas_metadb/migrations/V0270__Trigger_constraint_versions_updatable_to.sql
  echo '' > /opt/datacloud/dbaas_metadb/migrations/V0346__plpython3.sql
fi;

MIGRATION_TARGET=${MIGRATION_TARGET-latest}
pgmigrator --target "$MIGRATION_TARGET" \
    --conn "$DSN" \
    --base_dir /opt/datacloud/dbaas_metadb/

# use --refill if you want to begin from scratch
populate_table -c "$DSN" -f /etc/yandex/metadb/resources/default_pillar.yaml --table dbaas.default_pillar --key id
populate_table -c "$DSN" -f /etc/yandex/metadb/resources/region.yaml --table dbaas.regions
populate_table -c "$DSN" -f /etc/yandex/metadb/resources/geo.yaml --table dbaas.geo -k geo_id
populate_table -c "$DSN" -f /etc/yandex/metadb/resources/disk_type.yaml --table dbaas.disk_type -k disk_type_ext_id
populate_table -c "$DSN" -f /etc/yandex/metadb/resources/flavor_type.yaml --table dbaas.flavor_type -k id
populate_table -c "$DSN" -f /etc/yandex/metadb/resources/flavor.yaml --table dbaas.flavors -k id
populate_table -c "$DSN" -f /etc/yandex/metadb/resources/cluster_type.yaml --table dbaas.cluster_type_pillar -k type
populate_table -c "$DSN" -f /etc/yandex/metadb/resources/role_pillar.yaml --table dbaas.role_pillar --refill
populate_table -c "$DSN" -f /etc/yandex/metadb/resources/default_feature_flag.yaml --table dbaas.default_feature_flags -k flag_name
populate_table -c "$DSN" -f /etc/yandex/metadb/resources/default_version.yaml --table dbaas.default_versions -k type,component,name,env
populate_table -c "$DSN" -f /etc/yandex/metadb/resources/config_host_access_id.yaml --table dbaas.config_host_access_ids -k access_id --refill
VALID_RESOURCES_IN_HIGHSTATE=${VALID_RESOURCES_IN_HIGHSTATE-false}  # TODO: delete after https://st.yandex-team.ru/MDB-12429
if [[ "$VALID_RESOURCES_IN_HIGHSTATE" == "false" ]]; then
  vr_gen2 produce -d /etc/yandex/metadb/resources/ -o /opt/metadb/resources/valid_resources.yaml
  populate_table -c "$DSN" -f /opt/metadb/resources/valid_resources.yaml --table dbaas.valid_resources -k id --refill
fi;
USE_VR_GEN=${USE_VR_GEN-false}
if [[ "$USE_VR_GEN" == "true" ]]; then
  vr_gen /etc/yandex/metadb/resources /opt/metadb/resources --is-k8s
  populate_table -c "$DSN" -f /opt/metadb/resources/valid_resources.json --table dbaas.valid_resources -k id --refill
fi;
