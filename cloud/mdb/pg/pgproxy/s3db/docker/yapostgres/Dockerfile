FROM yaubuntu
MAINTAINER Dmitriy Sarafannikov <dsarafan@yandex-team.ru>

ARG pg_version=12

ENV PG_VERSION ${pg_version}

RUN apt-get update -qq \
    && apt-get install -y \
        postgresql-${pg_version} \
        postgresql-${pg_version}-replmon \
        postgresql-${pg_version}-partman \
        postgresql-client-${pg_version} \
        python-psycopg2

COPY entrypoint.sh /entrypoint.sh
COPY pg.conf /etc/postgresql/${pg_version}/main/
COPY pg_hba.conf /etc/postgresql/${pg_version}/main/
RUN echo "include 'pg.conf'"  >> /etc/postgresql/${pg_version}/main/postgresql.conf

EXPOSE 5432

USER postgres

CMD ["master"]
ENTRYPOINT ["/entrypoint.sh"]
