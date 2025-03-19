FROM cr.yandex/mirror/jaegertracing/jaeger-agent:1 AS ja

FROM cr.yandex/mirror/ubuntu:20.04 as ua-install

RUN echo "deb http://yandex-cloud-common-secure.dist.yandex.ru/yandex-cloud-common-secure stable/all/">/etc/apt/sources.list && \
    echo "deb http://yandex-cloud-common-secure.dist.yandex.ru/yandex-cloud-common-secure stable/amd64/" >>/etc/apt/sources.list && \
    apt update  -y --allow-insecure-repositories && apt install -y --allow-unauthenticated yandex-unified-agent

FROM cr.yandex/mirror/ubuntu:20.04

COPY files/apt-secure.list /etc/apt/sources.list
COPY --from=ja /go/bin/agent-linux /usr/bin/jaeger-agent
COPY --from=ua-install /usr/bin/unified_agent /usr/bin/unified_agent
COPY ua_config.yml /etc/yandex/unified_agent/config.yml
COPY tvmtool /usr/bin/tvmtool

RUN chmod a+x /usr/bin/jaeger-agent &&\
    chmod a+x /usr/bin/unified_agent &&\
    mkdir -p /data /unified-agent-conf.d

COPY files/unified_agent.yml /etc/yandex/unified_agent/config.yml
COPY configs /config-sources
COPY scripts/entrypoint.sh /entrypoint.sh
COPY version.txt /version.txt

ENTRYPOINT ["/entrypoint.sh"]
