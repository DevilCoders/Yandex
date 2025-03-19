#!/bin/bash

set -e

set -x

chown -R postgres:postgres /dist
mkdir -p /var/run/postgresql/${PG_MAJOR}-main.pg_stat_tmp
chown -R postgres:postgres /var/run/postgresql/
sed -i "s/#listen_addresses = 'localhost'/listen_addresses = '*'/g" /etc/postgresql/${PG_MAJOR}/main/postgresql.conf
echo "host all all 0.0.0.0/0 trust" >> /etc/postgresql/${PG_MAJOR}/main/pg_hba.conf
echo "host all all ::0/0 trust"  >> /etc/postgresql/${PG_MAJOR}/main/pg_hba.conf
retry sudo -u postgres /usr/lib/postgresql/${PG_MAJOR}/bin/pg_ctl -D /etc/postgresql/${PG_MAJOR}/main --wait start
while ! sudo -u postgres /usr/lib/postgresql/${PG_MAJOR}/bin/pg_isready;
do
    echo "PG is not ready"
done
cd /dist
while read -r i
do
    retry sudo -u postgres /usr/lib/postgresql/${PG_MAJOR}/bin/psql \
        -c "CREATE USER $i WITH PASSWORD '$i'"
done < <(cat grants/*.sql | awk '{print $NF};' | sed 's/\;//g; /^$/d' | sort -u)
retry sudo -u postgres /usr/lib/postgresql/${PG_MAJOR}/bin/psql \
    -c "CREATE DATABASE secretsdb"
retry sudo -u postgres /opt/yandex/pgmigrate/bin/pgmigrate migrate -vvv -c "host=/var/run/postgresql dbname=secretsdb"

echo "Migration completed"

echo "Not stopping, Ctrl+C to cancel"
sleep infinity
