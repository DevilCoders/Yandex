#!/bin/bash

set -e

DSN="postgresql://$PGUSER:$PGPASSWORD@$PGHOST:$PGPORT/$PGDATABASE?sslmode=verify-full&sslrootcert=/etc/datacloud/ca/db.pem&connect_timeout=5"

create_users --conn "$DSN" --user temporal --application temporal
