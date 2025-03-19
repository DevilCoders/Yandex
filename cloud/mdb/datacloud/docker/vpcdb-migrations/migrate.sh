#!/bin/bash
set -e

DSN="postgresql://$PGUSER:$PGPASSWORD@$PGHOST:$PGPORT/$PGDATABASE?sslmode=verify-full&sslrootcert=/etc/datacloud/ca/db.pem&connect_timeout=5"

create_users -c "$DSN" -u vpc_worker -a vpc -u vpc_api
MIGRATION_TARGET=${MIGRATION_TARGET-latest}
pgmigrate migrate --target "$MIGRATION_TARGET" \
    -c "$DSN" \
    --base_dir /opt/datacloud/vpcdb/ \
    --verbose
