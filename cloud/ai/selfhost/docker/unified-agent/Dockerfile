FROM ubuntu:20.04

USER root
RUN apt-get update -qq && \
    apt-get install --force-yes -y gnupg

ADD conf/*.list /etc/apt/sources.list.d/

RUN sed -i "s/archive.ubuntu.com/mirror.yandex.ru/g" /etc/apt/sources.list

RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 7FCD11186050CD1A && \
    apt-get update -qq && \
    apt-get install --force-yes -y \
        yandex-internal-root-ca \
        yandex-ca-certs \
        yandex-archive-keyring \
        yandex-conf-repo-prestable \
        yandex-conf-repo-testing \
        yandex-conf-repo-unstable \
        yandex-unified-agent \
        less \
        wget \
        curl \
        vim \
        lsof \
        htop \
        iotop \
        tcpdump \
        netcat \
        net-tools \
    && \
    apt-get --allow-unauthenticated clean && \
    ln -sf bash /bin/sh

ENTRYPOINT [ \
    "/usr/bin/unified_agent", \
    "--log-priority", "RESOURCES", \
    "--config", "/etc/yandex/unified_agent/config.yml" \
]
