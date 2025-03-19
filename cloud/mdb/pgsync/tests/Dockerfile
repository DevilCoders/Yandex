FROM ubuntu:bionic

ARG pg_major
ARG confpath="tests/conf"

ENV DEBIAN_FRONTEND noninteractive
ENV PG_MAJOR $pg_major
ENV PGSYNC_CONFIG /etc/pgsync.conf
ENV PGBOUNCER_CONFIG /etc/pgbouncer/pgbouncer.ini
ENV POSTGRESQL_DATADIR /var/lib/postgresql/$PG_MAJOR/main
ENV POSTGRESQL_CONFIG $POSTGRESQL_DATADIR/postgresql.conf
ENV POSTGRESQL_PGHBA $POSTGRESQL_DATADIR/pg_hba.conf

RUN echo 'APT::Install-Recommends "0"; \n\
APT::Get::Assume-Yes "true"; \n\
APT::Get::allow-downgrades "true"; \n\
APT::Install-Suggests "0";' > /etc/apt/apt.conf.d/01buildconfig

RUN echo "deb http://mirror.yandex.ru/ubuntu/ bionic main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list

RUN apt-get update && apt-get install wget gnupg ca-certificates

RUN wget --quiet -O - http://mirror.yandex.ru/mirrors/postgresql/ACCC4CF8.asc | apt-key add -

RUN echo 'deb http://dist.yandex.ru/mdb-bionic stable/all/' > /etc/apt/sources.list.d/yandex.list && \
    echo 'deb http://dist.yandex.ru/mdb-bionic stable/$(ARCH)/' >> /etc/apt/sources.list.d/yandex.list

RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 7FCD11186050CD1A

RUN apt-get update && apt-get -y install \
        postgresql-common=232.pgdg18.04+1+yandex0 \
        postgresql-client-common=232.pgdg18.04+1+yandex0
RUN echo "wal_level = replica" >> /etc/postgresql-common/createcluster.conf

RUN apt-get -y install \
        git \
        pgbouncer=1.16.1-1.pgdg18.04+1+yandex0 \
        postgresql-$PG_MAJOR \
        postgresql-$PG_MAJOR-replmon \
        postgresql-client-$PG_MAJOR \
        lwaldump-$PG_MAJOR \
        python3-dev \
        python3-pip \
        python3-venv \
        python3-setuptools \
        libpq-dev \
        gcc \
        lsof \
        sudo \
        rsync

RUN pip3 install \
        git+https://github.com/Supervisor/supervisor.git@2c601dbe1a09c98446dbff404d3f046d0c6a4cc9

RUN update-alternatives --install /usr/bin/python python /usr/bin/python3 1

COPY $confpath/postgresql.conf $POSTGRESQL_CONFIG
COPY $confpath/pg_hba.conf $POSTGRESQL_PGHBA
RUN chmod 666 $POSTGRESQL_PGHBA && chmod 666 $POSTGRESQL_CONFIG
RUN cp /etc/postgresql/$PG_MAJOR/main/pg_ident.conf $POSTGRESQL_DATADIR \
    && chown postgres:postgres $POSTGRESQL_DATADIR/pg_ident.conf
RUN ln -s /usr/lib/postgresql/$PG_MAJOR/bin /usr/bin/postgresql

USER postgres
RUN echo "CHECKPOINT;" | /usr/bin/postgresql/postgres --single \
        -D $POSTGRESQL_DATADIR postgres
RUN mkdir /var/lib/postgresql/$PG_MAJOR/main/conf.d
USER root

RUN mkdir /var/log/pgsync && chown postgres:postgres /var/log/pgsync && \
    mkdir -p /etc/pgsync/plugins && mkdir /var/log/supervisor && \
    mkdir -p /etc/pgbouncer

RUN echo '"postgres" ""' >> /etc/pgbouncer/userlist.txt

COPY $confpath/pgsync.conf $PGSYNC_CONFIG
COPY $confpath/pgbouncer.ini $PGBOUNCER_CONFIG
COPY $confpath/supervisord.conf /etc/supervisor/supervisord.conf
COPY $confpath/pgbouncer.supervisor /etc/supervisor/conf.d/pgbouncer.conf
COPY $confpath/pgsync.supervisor /etc/supervisor/conf.d/pgsync.conf
COPY $confpath/gen_rec_conf_with_slot.sh /usr/local/bin/
COPY $confpath/gen_rec_conf_without_slot.sh /usr/local/bin/

COPY $confpath/archive.passwd /etc
RUN chown postgres:postgres /etc/archive.passwd && chmod 600 /etc/archive.passwd
RUN echo "su - postgres -c '/usr/lib/postgresql/$PG_MAJOR/bin/pg_ctl -D /var/lib/postgresql/$PG_MAJOR/main/ promote'" > /usr/bin/promote > /usr/bin/promote && chmod +x /usr/bin/promote

COPY tests/setup.sh /usr/local/bin/setup.sh

RUN /usr/local/bin/setup.sh $PG_MAJOR

CMD ["/usr/local/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]

RUN mkdir /repo
COPY ./ /repo/
RUN cd /repo && cp ./ya_build/pgsync /usr/local/bin/ && cp ./ya_build/pgsync-util /usr/local/bin/
RUN ln -s /repo/src/plugins/upload_wals.py /etc/pgsync/plugins/upload_wals.py
