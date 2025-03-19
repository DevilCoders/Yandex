FROM cr.yandex/yc-internal/base:2019-12-13T12-18
MAINTAINER https://wiki.yandex-team.ru/solomon/

RUN apt-get update && \
apt-get install --force-yes -y yandex-solomon-agent-bin yc-solomon-agent-plugins yandex-solomon-sysmond

CMD ["/usr/local/bin/solomon-agent", "--config", "/etc/solomon-agent/agent.conf"]
