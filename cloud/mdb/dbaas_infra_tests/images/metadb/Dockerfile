FROM registry.yandex.net/ubuntu:bionic
LABEL image-cleanup=true

RUN echo "deb http://mirror.yandex.ru/ubuntu/ bionic main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list

RUN rm -f /etc/apt/sources.list.d/common-* \
    /etc/apt/sources.list.d/yandex-bionic-* \
    /etc/apt/sources.list.d/testing.list \
    /etc/apt/sources.list.d/prestable.list \
    /etc/apt/sources.list.d/unstable.list

ENV DEBIAN_FRONTEND noninteractive

RUN groupadd -r postgres --gid=999 && \
        useradd -r -d /var/lib/postgresql -g postgres --uid=999 postgres

# make the "en_US.UTF-8" locale so postgres will be utf-8 enabled by default
RUN apt-get update && apt-get install -y tzdata locales gnupg && \
    rm -rf /var/lib/apt/lists/* && \
    localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.utf8

ENV PG_MAJOR 12
ENV PG_VERSION 12.9-201-yandex.48887.c52a3f02f4


RUN echo 'deb http://dist.yandex.ru/mdb-bionic-secure stable/all/' > /etc/apt/sources.list.d/mdb-bionic-stable.list && \
    echo 'deb http://dist.yandex.ru/mdb-bionic-secure stable/$(ARCH)/' >> /etc/apt/sources.list.d/mdb-bionic-stable.list

RUN apt-get update -qq --allow-insecure-repositories \
    && apt-get install -y --allow-unauthenticated \
        postgresql-common \
        postgresql-$PG_MAJOR=$PG_VERSION \
        postgresql-plpython3-$PG_MAJOR=$PG_VERSION \
        postgresql-${PG_MAJOR}-plv8=2.5-2ed1271 \
        retry=1.6101280 \
        mdb-pgmigrate

COPY config/postgresql.conf /etc/postgresql/${PG_MAJOR}/main/conf.d/postgresql.conf
RUN echo "host all all 0.0.0.0/0 trust" >> /etc/postgresql/${PG_MAJOR}/main/pg_hba.conf
RUN echo "host all all ::0/0 trust"  >> /etc/postgresql/${PG_MAJOR}/main/pg_hba.conf

COPY ./data /dist

RUN chown -R postgres:postgres /dist

USER postgres
ENV PGDATA /etc/postgresql/${PG_MAJOR}/main
ENV PGBIN=/usr/lib/postgresql/${PG_MAJOR}/bin
ENV PATH $PATH:$PGBIN

# create user for each file in grants/
RUN ls /dist/head/grants/ | awk -F. '{printf "CREATE USER %s WITH PASSWORD '\''%s'\'';\n", $1, $1}' > /dist/create_users.sql
# add callback that install initial data after all migrations
RUN echo 'callbacks: {afterAll: [data]}' >> /dist/migrations.yml
RUN retry pg_ctl --wait start && \
    retry psql -c "CREATE DATABASE dbaas_metadb" && \
    retry psql -1 -f /dist/create_users.sql && \
    (cd /dist && retry /opt/yandex/pgmigrate/bin/pgmigrate migrate -t latest -vvv -c "dbname=dbaas_metadb") && \
    retry psql -c "GRANT SELECT ON schema_version TO dbaas_api;" dbaas_metadb && \
    pg_ctl --wait stop

EXPOSE 5432
CMD ["postgres"]
