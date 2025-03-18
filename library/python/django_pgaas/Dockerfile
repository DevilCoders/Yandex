FROM registry.yandex.net/tools/raw-ubuntu:18.04

WORKDIR /

# System dependencies
RUN apt-get update -q \
    && apt-get install -y --no-upgrade --no-install-recommends \
        gcc \
        python-dev \
        python3-dev \
        python-pip \
        python3-pip \
        python-tox \
        python-setuptools \
        libpq-dev \
        postgresql-client-10 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir /reports

# Copy & install procu with tests
COPY setup.py pytest.ini tox.ini README.md ./
COPY django_pgaas/ ./django_pgaas
COPY tests/ ./tests
