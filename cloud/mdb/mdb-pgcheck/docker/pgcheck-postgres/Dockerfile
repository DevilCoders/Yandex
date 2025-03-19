FROM ubuntu:bionic
MAINTAINER MDB Team <mdb@yandex-team.ru>

RUN echo "deb http://mirror.yandex.ru/ubuntu/ bionic main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list

ARG pg_version=10

ENV DEBIAN_FRONTEND noninteractive
ENV PG_VERSION ${pg_version}

# explicitly set user/group IDs
RUN groupadd -r postgres --gid=999 && useradd -r -d /var/lib/postgresql -g postgres --uid=999 postgres

# make the "en_US.UTF-8" locale so postgres will be utf-8 enabled by default
RUN apt-get update && apt-get install -y tzdata locales gnupg && \
    rm -rf /var/lib/apt/lists/* && \
    localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.utf8

COPY trusted_keys /tmp/trusted_keys

RUN apt-key add /tmp/trusted_keys/pgdg.gpg \
    && rm -rf /tmp/trusted_keys \
    && echo "deb http://apt.postgresql.org/pub/repos/apt/ bionic-pgdg main" > /etc/apt/sources.list.d/pgdg.list \
    && apt-get update -qq \
    && apt-get install -y \
        postgresql-${pg_version} \
        postgresql-contrib-${pg_version} \
        pgbouncer \
        sudo \
    && echo "postgres ALL=(ALL) NOPASSWD: ALL" >>/etc/sudoers

COPY entrypoint.sh /entrypoint.sh
COPY pg.conf /etc/postgresql/${pg_version}/main/
COPY pg_hba.conf /etc/postgresql/${pg_version}/main/
COPY pgbouncer.ini /etc/pgbouncer/pgbouncer.ini
COPY userlist.txt /etc/pgbouncer/userlist.txt
COPY fake.sql /tmp/fake.sql
RUN echo "include 'pg.conf'" >> /etc/postgresql/${pg_version}/main/postgresql.conf

EXPOSE 5432 6432

USER postgres

CMD ["master"]
ENTRYPOINT ["/entrypoint.sh"]
