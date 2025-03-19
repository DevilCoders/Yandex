FROM registry.yandex.net/ubuntu:bionic

LABEL maintainer="mdb@yandex-team.ru"

COPY install.bash /install.bash
RUN /bin/bash /install.bash

CMD ["/usr/bin/unified_agent", "--config", "/etc/yandex/unified_agent/config.yml"]
