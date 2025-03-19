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

# supervisor
COPY ./config/supervisord.conf /etc/supervisor/supervisord.conf

# resourcemanager-mock
COPY ./bin/resourcemanager-mock /resourcemanager-mock
COPY config/expectations.json /expectations.json
COPY config/cert.pem /cert.pem
COPY config/key.pem /key.pem
COPY config/resourcemanager-mock.supervisor.conf /etc/supervisor/conf.d/resourcemanager-mock.conf

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
