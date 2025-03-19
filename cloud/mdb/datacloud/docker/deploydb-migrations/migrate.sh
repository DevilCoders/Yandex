#!/bin/bash

set -e

HOSTSPEC=$(python3 -c "hosts = '$PGHOST'; port = '$PGPORT'; \
  print(','.join('{}:{}'.format(host, port) for host in hosts.split(',')))")

DSN="postgresql://$PGUSER:$PGPASSWORD@$HOSTSPEC/$PGDATABASE?sslmode=verify-full&sslrootcert=$PGSSLROOTCERT&target_session_attrs=read-write&connect_timeout=5"

if [[ -n "$IS_MASTER_REGION" ]] && [[ "$IS_MASTER_REGION" == "true" ]]; then
    CREATE_USERS=${CREATE_USERS-yes}
    if [[ "$CREATE_USERS" == "yes" ]]; then
      create_users -g /opt/datacloud/deploydb/grants -a deploy -c "$DSN"
    fi
    MIGRATION_TARGET=${MIGRATION_TARGET-latest}
    pgmigrate migrate \
        --target "$MIGRATION_TARGET" \
        -c "$DSN" \
        --base_dir /opt/datacloud/deploydb/ \
        --callbacks "afterAll:code/defs,afterAll:code/private,afterAll:code/api,afterAll:grants" \
        --verbose
fi

mdb-deploy-registrar --config-path=/config/ --masters="${MASTERS_TO_CREATE}" --group="${DEPLOY_GROUP_NAME}"
