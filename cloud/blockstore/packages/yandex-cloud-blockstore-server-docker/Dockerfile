FROM cr.yandex/yc-internal/ubuntu-build-dep:manual

WORKDIR /

COPY /blockstore-server /usr/bin/blockstore-server
COPY /blockstore-client /usr/bin/blockstore-client
COPY /configure-and-start.sh /usr/bin/configure-and-start.sh

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get -yqq update && \
    apt-get -yqq --allow-unauthenticated install yandex-archive-keyring && \
    apt-get -yqq upgrade && \
    apt-get -yqq install dnsutils telnet netcat-openbsd iputils-ping gdb python3-minimal yandex-internal-root-ca jq && \
    apt-get -yqq clean all && \
    rm -rf /var/lib/apt/lists/*

CMD ["bash", "-c", "source /Berkanavt/nbs-server/cfg/nbs_server.cfg && /usr/bin/blockstore-server ${nbs_arg}"]
