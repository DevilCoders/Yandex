FROM yaubuntu
MAINTAINER Dmitriy Sarafannikov <dsarafan@yandex-team.ru>

ARG pg_version=12

ENV PG_VERSION ${pg_version}

RUN apt-get update -qq \
    && apt-get install -y \
    postgresql-client-${PG_VERSION} \
    python-psycopg2 \
    python-pip \
    git \
    && pip install -U 'setuptools<45'

COPY requirements.txt .

RUN pip install -r requirements.txt

COPY entrypoint.sh /entrypoint.sh

CMD ["check"]
ENTRYPOINT ["/entrypoint.sh"]
