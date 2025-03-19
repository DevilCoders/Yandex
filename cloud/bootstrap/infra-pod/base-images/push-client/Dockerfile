FROM cr.yandex/yc-bootstrap/ubuntu:xenial-20210114-2.0

ENV YANDEX_PUSH_CLIENT_VERSION="6.64.0"
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y "yandex-push-client=${YANDEX_PUSH_CLIENT_VERSION}" \
    && rm -rf /var/lib/apt/lists/*

CMD ["/usr/bin/push-client", "-f", "-c", "/etc/yandex/statbox-push-client/push-client.yaml"]
