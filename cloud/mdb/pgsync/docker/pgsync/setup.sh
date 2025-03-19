#!/bin/bash
set -ex

PG_MAJOR=$1
MASTER=$2
PGDATA="/var/lib/postgresql/${PG_MAJOR}/main"

wait_pg() {
    tries=0
    ret=1
    while [ ${tries} -le 60 ]
    do
        if (echo "select 1" | su - postgres -c "psql --set ON_ERROR_STOP=1" >/dev/null 2>&1)
        then
            ret=0
            break
        else
            tries=$(( tries + 1 ))
            sleep 1
        fi
    done
    return ${ret}
}

make_config() {
    cat /root/postgresql.conf > ${PGDATA}/postgresql.conf
    cat /root/pg_hba.conf > ${PGDATA}/pg_hba.conf
    chown postgres:postgres ${PGDATA}/postgresql.conf ${PGDATA}/pg_hba.conf
    echo "include = '${PGDATA}/postgresql.conf'" > /etc/postgresql/${PG_MAJOR}/main/postgresql.conf
    echo "data_directory = '${PGDATA}'" >> /etc/postgresql/${PG_MAJOR}/main/postgresql.conf
    echo "hba_file = '${PGDATA}/pg_hba.conf'" >> ${PGDATA}/postgresql.conf
    echo "ident_file = '/etc/postgresql/${PG_MAJOR}/main/pg_ident.conf'" >> ${PGDATA}/postgresql.conf
    echo "include_if_exists = '${PGDATA}/conf.d/recovery.conf'" >> ${PGDATA}/postgresql.conf
}

supervisorctl stop pgsync

if [ "${MASTER}" = "" ]
then
    pg_createcluster ${PG_MAJOR} main -- -k
    make_config
    sudo -u postgres mkdir ${PGDATA}/conf.d
    pg_ctlcluster ${PG_MAJOR} main start && \
    if ! wait_pg
    then
        exit 1
    fi
    sudo -u postgres psql --set ON_ERROR_STOP=1 -c 'CREATE EXTENSION IF NOT EXISTS lwaldump'
    while :
    do
        echo "create user repl with encrypted password 'repl' replication superuser;" | su - postgres -c psql >/dev/null 2>&1
        supervisorctl start pgsync 2>/dev/null >/dev/null || supervisorctl status pgsync
        if psql --set ON_ERROR_STOP=1 -c 'CREATE TABLE IF NOT EXISTS set (value integer primary key)' "host=localhost port=6432 dbname=postgres user=repl" >/dev/null 2>&1
        then
            break
        else
            sleep 1
        fi
    done
else
    pg_createcluster $PG_MAJOR main
    make_config
    echo -n "Waiting while master is ready... "
    while :
    do
        psql --set ON_ERROR_STOP=1 -c 'select 1' "host=${MASTER} port=6432 dbname=postgres user=repl" >/dev/null 2>&1 && \
        if [ -f /tmp/pgsync_init ]
        then
            echo "trying to start pgsync"
            supervisorctl start pgsync 2>/dev/null >/dev/null || supervisorctl status pgsync
        else
            echo "starting setup"
            rm -rf ${PGDATA}/* && \
            (psql "host=${MASTER} port=6432 dbname=postgres user=repl" -c "select pg_drop_replication_slot('$(hostname -f | sed -e 's/\./_/g' -e 's/\-/_/g')');" >/dev/null 2>&1 || true) && \
            psql "host=${MASTER} port=6432 dbname=postgres user=repl" -c "select pg_create_physical_replication_slot('$(hostname -f | sed -e 's/\./_/g' -e 's/\-/_/g')');" >/dev/null 2>&1 || true && \
            su - postgres -c "pg_basebackup --pgdata=${PGDATA} --wal-method=fetch --dbname=\"host=${MASTER} port=5432 dbname=postgres user=repl\"" && \
            su - postgres -c "/usr/local/bin/gen_rec_conf.sh ${MASTER} ${PGDATA}/conf.d/recovery.conf" && \
            pg_ctlcluster $PG_MAJOR main start; \
            wait_pg && \
            touch /tmp/pgsync_init && \
            (supervisorctl start pgsync 2>/dev/null >/dev/null || supervisorctl status pgsync)
        fi
        if psql --set ON_ERROR_STOP=1 -c 'select 1' "host=localhost port=6432 dbname=postgres user=repl" >/dev/null 2>&1
        then
            break
        else
            sleep 1
        fi
    done
fi
