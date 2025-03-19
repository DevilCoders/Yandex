FROM ubuntu:bionic

RUN echo "deb http://mirror.yandex.ru/ubuntu/ bionic main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list

ENV DEBIAN_FRONTEND=noninteractive
ENV SALT_VERSION='3000.9+ds-1+yandex0'

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
        salt-minion=${SALT_VERSION} \
        salt-common=${SALT_VERSION} \
        supervisor

RUN locale-gen en_US.UTF-8 && update-locale LANG=en_US.UTF-8

COPY ./config/allCAs.pem /opt/yandex/allCAs.pem

# supervisor
COPY ./config/supervisord.conf /etc/supervisor/supervisord.conf

# salt-minion
COPY ./config/minion.supervisor.conf /etc/supervisor/conf.d/salt-minion.conf

# salt-minion config
COPY ./mdb-config-salt/minion /etc/salt/minion.d/yandex.conf
COPY ./config/zz_debug.conf /etc/salt/minion.d/zz_debug.conf

# salt-minion get_master
COPY ./mdb-config-salt/get_master.py /var/lib/salt/modules/get_master.py

# dbaas.py
COPY ./mdb-salt-modules/dbaas.py /var/lib/salt/modules/dbaas.py

# mdb-salt-returner
COPY ./mdb-config-salt/mdb_salt_returner.py /usr/local/yandex/salt/returners/mdb-salt-returner.py

# mdb-ping-salt-master and its config with fast options and its 'starter' for supervisor
COPY ./mdb-config-salt/mdb_ping_salt_master.py /opt/yandex/mdb-ping-salt-master/mdb-ping-salt-master.py
COPY ./config/mdb-ping-salt-master.yaml /etc/yandex/mdb-ping-salt-master/mdb-ping-salt-master.yaml
COPY ./config/mdb-ping-salt-master.supervisor.conf /etc/supervisor/conf.d/mdb-ping-salt-master.conf

COPY ./config/deploy_version /etc/yandex/mdb-deploy/deploy_version
COPY ./config/mdb_deploy_api_host /etc/yandex/mdb-deploy/mdb_deploy_api_host

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
