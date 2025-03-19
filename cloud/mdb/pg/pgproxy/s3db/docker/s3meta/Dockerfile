FROM yapostgres
MAINTAINER Dmitriy Sarafannikov <dsarafan@yandex-team.ru>

USER root

COPY s3meta.conf /etc/postgresql/${PG_VERSION}/main/
RUN echo "include 's3meta.conf'"  >> /etc/postgresql/${PG_VERSION}/main/postgresql.conf

USER postgres
