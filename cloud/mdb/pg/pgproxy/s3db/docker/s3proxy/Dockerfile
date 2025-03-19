FROM yapostgres
MAINTAINER Dmitriy Sarafannikov <dsarafan@yandex-team.ru>

USER root

RUN apt-get update -qq \
    && apt-get install -y --force-yes \
        postgresql-${PG_VERSION}-plproxy

USER postgres
