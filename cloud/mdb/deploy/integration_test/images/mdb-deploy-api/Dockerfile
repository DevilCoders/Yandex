FROM ubuntu:bionic

RUN echo "deb http://mirror.yandex.ru/ubuntu/ bionic main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list

ENV DEBIAN_FRONTEND=noninteractive

RUN echo 'deb http://dist.yandex.ru/mdb-bionic stable/all/' > /etc/apt/sources.list.d/mdb-bionic-stable.list && \
        echo 'deb http://dist.yandex.ru/mdb-bionic stable/$(ARCH)/' >> /etc/apt/sources.list.d/mdb-bionic-stable.list && \
        echo 'deb http://dist.yandex.ru/mdb-bionic unstable/all/' > /etc/apt/sources.list.d/mdb-bionic-unstable.list && \
        echo 'deb http://dist.yandex.ru/mdb-bionic unstable/$(ARCH)/' >> /etc/apt/sources.list.d/mdb-bionic-unstable.list && \
        echo 'Package: *' > /etc/apt/preferences.d/01dist-pin-priority && \
        echo 'Pin: origin "dist.yandex.ru"' >> /etc/apt/preferences.d/01dist-pin-priority && \
        echo 'Pin-Priority: 1001' >> /etc/apt/preferences.d/01dist-pin-priority && \
        apt-get update -qq -y --allow-insecure-repositories && \
        apt-get install -y --allow-unauthenticated yandex-archive-keyring && \
        apt-get update -qq && apt-get upgrade -y && apt-get install -y \
        locales \
        util-linux \
        iputils-ping \
        net-tools \
        telnet \
        traceroute \
        dnsutils \
        whois \
        curl \
        wget \
        gnupg \
        lsof \
        less \
        vim \
        nano \
        sudo \
        mc \
        nginx-common \
        nginx-full \
        supervisor \
        yandex-passport-tvmtool

RUN locale-gen en_US.UTF-8 && update-locale LANG=en_US.UTF-8

COPY ./config/allCAs.pem /opt/yandex/allCAs.pem

# supervisor
COPY ./config/supervisord.conf /etc/supervisor/supervisord.conf

# nginx
COPY ./config/nginx.supervisor.conf /etc/supervisor/conf.d/nginx.conf
RUN mkdir -p /etc/nginx/ssl
COPY ./config/dhparam.pem /etc/nginx/ssl/dhparam.pem
COPY ./config/nginx.conf /etc/nginx/nginx.conf

# mdb-deploy-api
COPY ./bin/mdb-deploy-api /mdb-deploy-api
COPY ./config/mdb-deploy-api.supervisor.conf /etc/supervisor/conf.d/mdb-deploy-api.conf
COPY ./config/mdb-deploy-api.yaml /mdb-deploy-api.yaml
COPY ./config/dbpg.yaml /dbpg.yaml
COPY ./config/mdb-deploy-api.nginx.conf /etc/nginx/conf.d/mdb-deploy-api.conf
COPY ./config/mdb-deploy-api.pem /etc/nginx/ssl/mdb-deploy-api.pem
COPY ./config/mdb-deploy-api.key /etc/nginx/ssl/mdb-deploy-api.key

# TVM tool
COPY ./config/tvmtool.conf /etc/tvmtool/tvmtool.conf
COPY ./config/tvmtool.supervisor.conf /etc/supervisor/conf.d/tvmtool.conf

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
