#!/bin/bash

set -e

set -x

chown -R postgres:postgres /dist
mkdir -p /var/run/postgresql/${PG_MAJOR}-main.pg_stat_tmp
chown postgres:postgres /var/run/postgresql/${PG_MAJOR}-main.pg_stat_tmp
sed -i "s/#listen_addresses = 'localhost'/listen_addresses = '*'/g" /etc/postgresql/${PG_MAJOR}/main/postgresql.conf
sed -i "s/#fsync = on/fsync = off/g" /etc/postgresql/${PG_MAJOR}/main/postgresql.conf
sed -i "s/#synchronous_commit = on/synchronous_commit = off/g" /etc/postgresql/${PG_MAJOR}/main/postgresql.conf
sed -i "s/#full_page_writes = on/full_page_writes = off/g" /etc/postgresql/${PG_MAJOR}/main/postgresql.conf
echo "host all all 0.0.0.0/0 trust" >> /etc/postgresql/${PG_MAJOR}/main/pg_hba.conf
echo "host all all ::0/0 trust"  >> /etc/postgresql/${PG_MAJOR}/main/pg_hba.conf
sudo -u postgres /usr/lib/postgresql/${PG_MAJOR}/bin/pg_ctl -D /etc/postgresql/${PG_MAJOR}/main start
while ! sudo -u postgres /usr/lib/postgresql/${PG_MAJOR}/bin/pg_isready;
do
    echo "PG is not ready"
done
cd /dist
while read -r i
do
    sudo -u postgres /usr/lib/postgresql/${PG_MAJOR}/bin/psql \
        -c "CREATE USER $i WITH PASSWORD '$i'"
done < <(grep -i '^grant' grants/*.sql | awk '{print $NF};' | sed 's/\;//g; /^$/d' | sort -u)
sudo -u postgres /usr/lib/postgresql/${PG_MAJOR}/bin/psql \
    -c "CREATE DATABASE dbaas_metadb"
sudo -u postgres /opt/yandex/pgmigrate/bin/pgmigrate migrate -vvv -c "host=/var/run/postgresql dbname=dbaas_metadb"

if [ "$NOSTOP" == 1 ]
then
    echo "Not stopping due to NOSTOP being set, Ctrl+C to cancel"
    sleep infinity
fi
echo "Stopping due to NOSTOP not being set"
