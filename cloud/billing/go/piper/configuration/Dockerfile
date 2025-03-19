FROM cr.yandex/mirror/ubuntu:20.04

RUN mkdir -p /config &&\
    mkdir -p /unified-agent-conf.d

COPY configs /config-sources
COPY scripts/entrypoint.sh /entrypoint.sh
COPY version.txt /version.txt

CMD ["/entrypoint.sh"]
