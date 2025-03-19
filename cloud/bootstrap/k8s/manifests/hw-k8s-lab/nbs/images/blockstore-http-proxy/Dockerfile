FROM cr.yandex/yc-bootstrap/ubuntu:xenial-20210114-2.0

ENV BLOCKSTORE_HTTP_PROXY_VERSION="49.tags.releases.nbs.stable-20-4-15-1"
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y "yandex-cloud-blockstore-http-proxy=${BLOCKSTORE_HTTP_PROXY_VERSION}" \
    && rm -rf /var/lib/apt/lists/*
