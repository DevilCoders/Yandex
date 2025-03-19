FROM cr.yandex/yc-bootstrap/ubuntu:xenial-20210114-2.0

ENV YANDEX_SOLOMON_AGENT_BIN_VERSION="1:17.1"
ENV YC_SOLOMON_AGENT_PLUGINS="0.5.1-242.210203"
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        "yandex-solomon-agent-bin=${YANDEX_SOLOMON_AGENT_BIN_VERSION}" \
        "yc-solomon-agent-plugins=${YC_SOLOMON_AGENT_PLUGINS}" \
    && rm -rf /var/lib/apt/lists/*

CMD ["solomon-agent", "--config", "/etc/yandex/solomon-agent/agent.conf"]
