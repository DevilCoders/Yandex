FROM registry.yandex.net/ubuntu:bionic

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

RUN apt-get update \
    && apt-get install -y postgresql-common \
        sudo \
        postgresql-$PG_MAJOR=$PG_VERSION \
        retry=1.6101280 \
        mdb-pgmigrate

COPY ./ /dist

EXPOSE 5432

CMD ["/dist/run.sh"]
