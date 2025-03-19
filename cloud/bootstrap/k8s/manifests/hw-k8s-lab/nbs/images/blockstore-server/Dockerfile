FROM cr.yandex/yc-bootstrap/ubuntu:xenial-20210114-2.0

ENV BLOCKSTORE_SERVER_VERSION="57.tags.releases.nbs.stable-20-6-3-1"
ENV BLOCKSTORE_CLIENT_VERSION="57.tags.releases.nbs.stable-20-6-3-1"
ENV BLOCKSTORE_NBD_VERSION="57.tags.releases.nbs.stable-20-6-3-1"

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y "yandex-cloud-blockstore-server=${BLOCKSTORE_SERVER_VERSION}" jq \
    "yandex-cloud-blockstore-nbd=${BLOCKSTORE_NBD_VERSION}" \
    "yandex-cloud-blockstore-client=${BLOCKSTORE_CLIENT_VERSION}" \
    && rm -rf /var/lib/apt/lists/*
