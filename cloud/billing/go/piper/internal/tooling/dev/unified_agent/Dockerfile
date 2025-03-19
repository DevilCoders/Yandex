FROM ubuntu:bionic


RUN apt-get update -qq \
    && apt-get upgrade -y \
    && apt-get install -y \
    locales \
    util-linux \
    iputils-ping \
    net-tools \
    telnet \
    traceroute \
    dnsutils \
    whois \
    curl \
    gnupg \
    less \
    && locale-gen en_US.UTF-8 && update-locale LANG=en_US.UTF-8

COPY yandex-bionic.list /etc/apt/sources.list.d/

RUN curl http://dist.yandex.ru/REPO.asc | apt-key add - \
    && apt-get update -qq \
    && apt-get install -y yandex-unified-agent

RUN mkdir -p /data
COPY ./entrypoint.sh /entrypoint.sh
RUN chmod 755 /entrypoint.sh

COPY ./config.yml /etc/yandex/unified_agent/config.tmpl.yml

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/usr/bin/unified_agent", "--config", "/etc/yandex/unified_agent/config.yml"]
