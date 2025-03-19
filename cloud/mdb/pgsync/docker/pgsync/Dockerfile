FROM pgsyncbase:latest
ARG replication_type
ARG pg_major
ARG pg_version

ENV PG_MAJOR $pg_major
ENV PG_VERSION $pg_version

RUN apt-get update && \
    apt-get install \
        libpq5=$PG_VERSION \
        postgresql-$PG_MAJOR=$PG_VERSION \
        postgresql-client-$PG_MAJOR=$PG_VERSION \
        postgresql-common=232.pgdg18.04+1+yandex0 \
        postgresql-client-common=232.pgdg18.04+1+yandex0 \
        build-essential \
        python-daemon \
        python-psycopg2 \
        python-setuptools \
        python-kazoo \
        pgbouncer=1.16.1-1.pgdg18.04+1+yandex0 \
        lwaldump-$PG_MAJOR && \
    pg_dropcluster --stop $PG_MAJOR main && \
    ln -s /usr/lib/postgresql/$PG_MAJOR/bin /usr/bin/postgresql && \
    mkdir -p /etc/pgsync/plugins && \
    cd /var/lib/dist && \
    cp /var/lib/dist/ya_build/pgsync /usr/local/bin/ && \
    cp /var/lib/dist/ya_build/pgsync-util /usr/local/bin/ && \
    cp /var/lib/dist/docker/pgsync/pgsync_${replication_type}.conf /etc/pgsync.conf && \
    cp /var/lib/dist/docker/pgsync/gen_rec_conf.sh /usr/local/bin/gen_rec_conf.sh && \
    echo "*:*:*:repl:repl" > /var/lib/postgresql/.pgpass && \
    chmod 600 /var/lib/postgresql/.pgpass && \
    chown postgres:postgres /var/lib/postgresql/.pgpass && \
    echo "*:*:*:repl:repl" > /root/.pgpass && \
    chmod 600 /root/.pgpass && \
    mkdir -p /etc/pgbouncer && \
    cp /var/lib/dist/docker/pgsync/postgresql.conf /root/postgresql.conf && \
    cp /var/lib/dist/docker/pgsync/pg_hba.conf /root/pg_hba.conf && \
    cp /var/lib/dist/docker/pgsync/supervisor.conf /etc/supervisor/conf.d/pgsync.conf && \
    cp /var/lib/dist/docker/pgsync/pgbouncer.ini /etc/pgbouncer/pgbouncer.ini && \
    cp /var/lib/dist/docker/pgsync/userlist.txt /etc/pgbouncer/userlist.txt && \
    cp /var/lib/dist/docker/pgsync/sudoers /etc/sudoers.d/pgsync && \
    cp /var/lib/dist/docker/pgsync/setup.sh /usr/local/bin/setup.sh
