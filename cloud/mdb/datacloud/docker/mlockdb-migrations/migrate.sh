#!/bin/bash

set -e

HOSTSPEC=$(python3 -c "hosts = '$PGHOST'; port = '$PGPORT'; \
  print(','.join('{}:{}'.format(host, port) for host in hosts.split(',')))")

DSN="postgresql://$PGUSER:$PGPASSWORD@$HOSTSPEC/$PGDATABASE?sslmode=verify-full&sslrootcert=$PGSSLROOTCERT&target_session_attrs=read-write&connect_timeout=5"

MIGRATION_TARGET=${MIGRATION_TARGET-latest}
pgmigrate migrate \
    --target "$MIGRATION_TARGET" \
    -c "$DSN" \
    --base_dir /opt/datacloud/mlockdb/ \
    --callbacks "afterAll:code,afterAll:grants" \
    --verbose
