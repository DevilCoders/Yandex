FROM ubuntu:bionic

RUN echo "deb http://mirror.yandex.ru/ubuntu/ bionic main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list

ENV DEBIAN_FRONTEND=noninteractive
ENV SALT_VERSION='3001.7+ds-1+yandex0'

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
        python-requests \
        nginx-common \
        nginx-full \
        python-cherrypy3 \
        python3-cherrypy3 \
        python3-nacl \
        python3-jwt \
        python3-paramiko \
        salt-master=${SALT_VERSION} \
        salt-api=${SALT_VERSION} \
        salt-common=${SALT_VERSION} \
        supervisor

RUN locale-gen en_US.UTF-8 && update-locale LANG=en_US.UTF-8

COPY ./config/allCAs.pem /opt/yandex/allCAs.pem

RUN useradd saltapi
RUN echo saltapi:testpwd | chpasswd

# supervisor
COPY ./config/supervisord.conf /etc/supervisor/supervisord.conf

# Docker IPv6 /etc/hosts hostname fix
COPY ./bin/ipv6_etc_hosts.sh /ipv6_etc_hosts.sh
COPY ./config/ipv6_etc_hosts.supervisor.conf /etc/supervisor/conf.d/ipv6_etc_hosts.conf

# nginx
COPY ./config/nginx.supervisor.conf /etc/supervisor/conf.d/nginx.conf
RUN mkdir -p /etc/nginx/ssl
COPY ./config/dhparam.pem /etc/nginx/ssl/dhparam.pem
COPY ./config/nginx.conf /etc/nginx/nginx.conf

# salt-master
COPY ./config/master.supervisor.conf /etc/supervisor/conf.d/salt-master.conf
COPY ./config/master /etc/salt/master

# salt-master key pair
COPY ./config/master.pem /etc/salt/pki/master/master.pem
COPY ./config/master.pub /etc/salt/pki/master/master.pub

# salt-master message signing key pair
COPY ./config/master_sign.pem /etc/salt/pki/master/master_sign.pem
COPY ./config/master_sign.pub /etc/salt/pki/master/master_sign.pub

# salt-api
COPY ./config/salt-api.supervisor.conf /etc/supervisor/conf.d/salt-api.conf
COPY ./config/salt-api.nginx.conf /etc/nginx/conf.d/salt-api.conf
COPY ./config/salt-api.pem /etc/nginx/ssl/salt-api.pem
COPY ./config/salt-api.key /etc/nginx/ssl/salt-api.key

# mdb-deploy-saltkeys
COPY ./bin/mdb-deploy-saltkeys /mdb-deploy-saltkeys
COPY ./config/mdb-deploy-saltkeys.supervisor.conf /etc/supervisor/conf.d/mdb-deploy-saltkeys.conf
COPY ./config/mdb-deploy-saltkeys.yaml /mdb-deploy-saltkeys.yaml

# salt data
COPY ./srv/ /srv/

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
