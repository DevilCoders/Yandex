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
        git \
        openssl \
        openssh-client \
        python3-pip \
        python3-setuptools \
        python3-virtualenv \
        python3-dev \
        build-essential \
        netcat-openbsd \
        dnsutils

RUN locale-gen en_US.UTF-8 && update-locale LANG=en_US.UTF-8

ENV LANG en_US.utf8

COPY requirements.txt /tmp/
RUN pip3 install --upgrade pip==9.0.3 && \
    pip3 install -r /tmp/requirements.txt

RUN mkdir -p /opt/yandex
COPY supervisord.conf /etc/supervisor/supervisord.conf

RUN mkdir /tmp/base_image
COPY src /tmp/base_image/src
RUN cd /tmp/base_image/ && \
    pip3 install --upgrade pip==9.0.3 && \
    pip3 install src/server_mock/ && \
    rm -rf /tmp/base_image


COPY src/fake_passport.py /opt/yandex/fake_passport.py
COPY src/formatters.py /opt/yandex/formatters.py
COPY src/methods.py /opt/yandex/methods.py
COPY config.py /opt/yandex/config.py
COPY uwsgi.ini /opt/yandex/uwsgi.ini
COPY nginx.conf /etc/nginx/nginx.conf
COPY supervisor.d /etc/supervisor/conf.d

EXPOSE 80 443

CMD ["supervisord", "-c", "/etc/supervisor/supervisord.conf"]
