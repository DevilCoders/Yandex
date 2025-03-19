ARG PYTHON_VERSION=3.10
ARG PYTHON_IMAGE_VERSION=${PYTHON_VERSION}-slim

FROM cr.yandex/mirror/python:$PYTHON_IMAGE_VERSION

WORKDIR /mr_prober

RUN python -m pip install --upgrade pip

RUN apt-get update && apt-get install -y \
      wget \
      gcc \
    && rm -rf /var/lib/apt/lists/*

RUN wget -O /etc/ssl/certs/YandexInternalRootCA.pem https://crls.yandex.net/YandexInternalRootCA.crt

RUN wget -O /usr/local/bin/skm https://storage.cloud-preprod.yandex.net/skm/linux/skm && \
  chmod +x /usr/local/bin/skm

COPY requirements.production.txt requirements.txt tools/routing_bingo/requirements.routing_bingo.txt  ./
RUN pip install -r requirements.routing_bingo.txt --no-cache-dir

COPY . .

ENV PYTHONPATH=/mr_prober
