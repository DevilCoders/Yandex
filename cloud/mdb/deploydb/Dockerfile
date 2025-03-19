# vim:set ft=dockerfile:
FROM ubuntu:bionic

RUN echo "deb http://mirror.yandex.ru/ubuntu/ bionic main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list

# explicitly set user/group IDs
RUN groupadd -r postgres --gid=999 && useradd -r -d /var/lib/postgresql -g postgres --uid=999 postgres

# make the "en_US.UTF-8" locale so postgres will be utf-8 enabled by default
RUN apt-get update && apt-get install -y tzdata locales gnupg && \
    rm -rf /var/lib/apt/lists/* && \
    localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.utf8

ENV PG_MAJOR 10
ENV PG_VERSION 10.16-201-yandex.45042.b5c6761944

RUN echo 'deb http://dist.yandex.ru/mdb-bionic stable/all/' > /etc/apt/sources.list.d/mdb-bionic-stable.list && \
    echo 'deb http://dist.yandex.ru/mdb-bionic stable/$(ARCH)/' >> /etc/apt/sources.list.d/mdb-bionic-stable.list

RUN apt-get -o Acquire::AllowInsecureRepositories=true \
    -o Acquire::AllowDowngradeToInsecureRepositories=true update \
    && apt-get -o APT::Get::AllowUnauthenticated=true \
    -y install yandex-archive-keyring \
    && apt-get update \
    && apt-get install -y postgresql-common \
        sudo \
        postgresql-$PG_MAJOR=$PG_VERSION \
        mdb-pgmigrate \
    && apt-get clean

RUN sed -i "s/#listen_addresses = 'localhost'/listen_addresses = '*'/g" /etc/postgresql/${PG_MAJOR}/main/postgresql.conf
RUN echo "host all all 0.0.0.0/0 trust" >> /etc/postgresql/${PG_MAJOR}/main/pg_hba.conf
RUN echo "host all all ::0/0 trust"  >> /etc/postgresql/${PG_MAJOR}/main/pg_hba.conf

COPY ./ /dist
RUN chown -R postgres:postgres /dist

USER postgres
ENV PGDATA /etc/postgresql/${PG_MAJOR}/main
ENV PGBIN=/usr/lib/postgresql/${PG_MAJOR}/bin
ENV PATH $PATH:$PGBIN

# create user for each file in grans/
RUN ls /dist/grants/ | awk -F. '{printf "CREATE USER %s WITH PASSWORD '\''%s'\'';\n", $1, $1}' > /dist/create_users.sql
RUN pg_ctl --wait start && \
    psql -c "CREATE DATABASE deploydb" && \
    psql -f /dist/create_users.sql && \
    (cd /dist && /opt/yandex/pgmigrate/bin/pgmigrate migrate -c "host=/var/run/postgresql dbname=deploydb" -t latest -vvv) && \
    pg_ctl --wait stop

EXPOSE 5432
CMD ["postgres"]
