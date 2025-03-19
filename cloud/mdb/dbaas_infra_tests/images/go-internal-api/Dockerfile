FROM registry.yandex.net/ubuntu:bionic
LABEL image-cleanup=true

RUN echo "deb http://mirror.yandex.ru/ubuntu/ bionic main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list

RUN rm -f /etc/apt/sources.list.d/common-* \
    /etc/apt/sources.list.d/yandex-bionic-* \
    /etc/apt/sources.list.d/testing.list \
    /etc/apt/sources.list.d/prestable.list \
    /etc/apt/sources.list.d/unstable.list

RUN apt-get update -qq -y --allow-insecure-repositories && \
        apt-get install -y \
        supervisor

COPY ./config/CA.pem /opt/yandex/mdb-infra-test-allCAs.pem
RUN mkdir /var/log/mdb-internal-api

# supervisor
COPY ./config/supervisord.conf /etc/supervisor/supervisord.conf

# mdb-internal-api
COPY ./bin/mdb-internal-api /mdb-internal-api
COPY config/go-internal-api.supervisor.conf /etc/supervisor/conf.d/mdb-internal-api.conf
COPY config/mdb-internal-api.yaml /mdb-internal-api.yaml
COPY config/console_default_resources.yaml /console_default_resources.yaml

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
