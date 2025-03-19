FROM cr.yandex/yc-bootstrap/ubuntu:xenial-20210114-2.0

ENV JUGGLER_CLIENT_VERSION="2.3.2001221352"
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y "juggler-client=${JUGGLER_CLIENT_VERSION}" \
    && rm -rf /var/lib/apt/lists/*

ADD juggler-client.default /etc/default/juggler-client

CMD ["/usr/bin/juggler-client"]
