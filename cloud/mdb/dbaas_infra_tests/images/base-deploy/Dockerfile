# vim:set ft=dockerfile:
FROM registry.yandex.net/ubuntu:bionic
LABEL image-cleanup=false

RUN echo "deb http://mirror.yandex.ru/ubuntu/ bionic main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list

RUN rm -f /etc/apt/sources.list.d/common-* \
        /etc/apt/sources.list.d/yandex-bionic-* \
        /etc/apt/sources.list.d/testing.list \
        /etc/apt/sources.list.d/prestable.list \
        /etc/apt/sources.list.d/unstable.list

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
        mc

RUN locale-gen en_US.UTF-8 && update-locale LANG=en_US.UTF-8
