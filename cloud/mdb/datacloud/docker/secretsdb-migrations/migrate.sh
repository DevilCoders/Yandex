#!/bin/bash

set -e

HOSTSPEC=$(python3 -c "hosts = '$PGHOST'; port = '$PGPORT'; \
  print(','.join('{}:{}'.format(host, port) for host in hosts.split(',')))")

DSN="postgresql://$PGUSER:$PGPASSWORD@$HOSTSPEC/$PGDATABASE?sslmode=verify-full&sslrootcert=$PGSSLROOTCERT&target_session_attrs=read-write&connect_timeout=5"

CREATE_USERS=${CREATE_USERS-yes}
if [[ "$CREATE_USERS" == "yes" ]]; then
  create_users -g /opt/datacloud/secretsdb/grants -a secrets -c "$DSN"
fi

MIGRATION_TARGET=${MIGRATION_TARGET-latest}
pgmigrate migrate \
    --target "$MIGRATION_TARGET" \
    -c "$DSN" \
    --base_dir /opt/datacloud/secretsdb/ \
    --callbacks "afterAll:grants" \
    --verbose
