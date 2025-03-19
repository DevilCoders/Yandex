#!/bin/bash
if [ "$1" = 'bash' ]
then
    /bin/bash
elif [ "$1" = 'master' ]
then
    /usr/lib/postgresql/${PG_VERSION}/bin/postgres \
        -D /var/lib/postgresql/${PG_VERSION}/main/ \
        -c config_file=/etc/postgresql/${PG_VERSION}/main/postgresql.conf
elif [ "$1" = 'replica' ]
then
    while ! psql -h $2 -c "select 1" > /dev/null
    do
        echo "Master not started yet. Sleep 1"
        sleep 1
    done;
    rm -rf /var/lib/postgresql/${PG_VERSION}/main/
    /usr/lib/postgresql/${PG_VERSION}/bin/pg_basebackup \
        -D /var/lib/postgresql/${PG_VERSION}/main/ \
        --write-recovery-conf \
        --wal-method=fetch \
        -h $2
    /usr/lib/postgresql/${PG_VERSION}/bin/postgres \
        -D /var/lib/postgresql/${PG_VERSION}/main/ \
        -c config_file=/etc/postgresql/${PG_VERSION}/main/postgresql.conf
else
    eval "$@"
fi
