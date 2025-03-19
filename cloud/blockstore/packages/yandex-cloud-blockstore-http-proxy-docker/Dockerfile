FROM cr.yandex/yc-internal/ubuntu-build-dep:manual

WORKDIR /

COPY /blockstore-http-proxy /usr/bin/blockstore-http-proxy

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get -yqq update && \
    apt-get -yqq --allow-unauthenticated install yandex-archive-keyring && \
    apt-get -yqq upgrade && \
    apt-get -yqq install dnsutils telnet netcat-openbsd iputils-ping gdb python3-minimal yandex-internal-root-ca jq && \
    apt-get -yqq clean all && \
    rm -rf /var/lib/apt/lists/*

CMD ["bash", "-c", "/usr/bin/blockstore-http-proxy --config=/Berkanavt/nbs-server/cfg/nbs-http-proxy.txt"]
