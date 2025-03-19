FROM cr.yandex/yc-internal/base:2019-12-13T12-18
MAINTAINER https://wiki.yandex-team.ru/solomon/

RUN apt-get update && \
apt-get install --force-yes -y yandex-solomon-agent-bin yc-solomon-agent-plugins yandex-solomon-sysmond

CMD ["/usr/local/bin/solomon-agent", "--config", "/etc/solomon-agent/agent.conf"]

#FROM cr.yandex/mirror/ubuntu:20.04
#
#ENV TZ=Europe/Moscow
#RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
#ENV export DEBIAN_FRONTEND="noninteractive"
#
#RUN echo "" >/etc/apt/sources.list && \
#    echo "deb http://mirror.yandex.ru/ubuntu/ focal main restricted">>/etc/apt/sources.list && \
#    echo "deb http://mirror.yandex.ru/ubuntu/ focal-updates main restricted">>/etc/apt/sources.list && \
#    echo "deb http://mirror.yandex.ru/ubuntu/ focal universe">>/etc/apt/sources.list && \
#    echo "deb http://mirror.yandex.ru/ubuntu/ focal-updates universe">>/etc/apt/sources.list && \
#    apt-get update && apt-get install -yqq \
#        curl less jq vim nano mc wget gnupg ca-certificates
#
#RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 7FCD11186050CD1A
#
#RUN echo "deb http://yandex-cloud.dist.yandex.ru/yandex-cloud stable/all/" >> /etc//apt/sources.list && \
#    apt-get update && apt-get install -yqq yandex-solomon-agent-bin yc-solomon-agent-plugins
#
###RUN apt-get install -yqq "yandex-solomon-agent-bin=${YANDEX_SOLOMON_AGENT_BIN_VERSION}"
###        "yc-solomon-agent-plugins=${YC_SOLOMON_AGENT_PLUGINS}"
##
#CMD ["solomon-agent", "--config", "/etc/yandex/solomon-agent/agent.conf"]
##
#FROM cr.yandex/yc-bootstrap/ubuntu:xenial-20210114-2.0
#
#ENV YANDEX_SOLOMON_AGENT_BIN_VERSION="1:17.1"
#ENV YC_SOLOMON_AGENT_PLUGINS="0.5.1-242.210203"
#RUN apt-get update \
#    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
#        "yandex-solomon-agent-bin=${YANDEX_SOLOMON_AGENT_BIN_VERSION}" \
#        "yc-solomon-agent-plugins=${YC_SOLOMON_AGENT_PLUGINS}" \
#    && rm -rf /var/lib/apt/lists/*
#
#CMD ["solomon-agent", "--config", "/etc/yandex/solomon-agent/agent.conf"]
