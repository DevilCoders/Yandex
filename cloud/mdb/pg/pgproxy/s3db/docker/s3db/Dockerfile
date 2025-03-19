FROM yapostgres
MAINTAINER Dmitriy Sarafannikov <dsarafan@yandex-team.ru>

USER root

COPY s3db.conf /etc/postgresql/${PG_VERSION}/main/
RUN echo "include 's3db.conf'"  >> /etc/postgresql/${PG_VERSION}/main/postgresql.conf

USER postgres
