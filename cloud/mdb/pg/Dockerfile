FROM yapostgres
MAINTAINER Dmitriy Sarafannikov <dsarafan@yandex-team.ru>

USER root

RUN apt-get update -qq \
    && apt-get install -y \
        postgresql-${PG_VERSION}-partman \
        postgresql-${PG_VERSION}-pathman \
        python-future \
        yamail-pgmigrate \
        python-yaml \
        make \
        git

RUN echo "shared_preload_libraries = 'pg_pathman'" >> /etc/postgresql/${PG_VERSION}/main/postgresql.conf

USER postgres

ENTRYPOINT /etc/init.d/postgresql start && cd /pg && WITHOUT_S3=1 make
