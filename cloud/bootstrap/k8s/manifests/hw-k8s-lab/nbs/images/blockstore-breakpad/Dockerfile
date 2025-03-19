FROM cr.yandex/yc-bootstrap/ubuntu:xenial-20210114-2.0

ENV BLOCKSTORE_BREAKPAD_VERSION="7053557.trunk.712502663"

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y "yandex-cloud-blockstore-breakpad=${BLOCKSTORE_BREAKPAD_VERSION}" \
    && rm -rf /var/lib/apt/lists/*


