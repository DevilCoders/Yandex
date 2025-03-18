FROM ubuntu:xenial
COPY ./etc /etc
COPY ./usr /usr
# install yandex certs/keys and tools
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 7FCD11186050CD1A
RUN apt-get update -qq && \
    apt-get install -y yandex-internal-root-ca yandex-archive-keyring yandex-ca-certs curl less vim net-tools telnet dnsutils iputils-ping apache2-utils tcpdump cron rsync apt-transport-https yandex-unbound && \
    apt-get clean

# set Europe/Moscow timezone
ENV TZ Europe/Moscow
RUN echo $TZ > /etc/timezone && \
    apt-get update -qq && apt-get install -y tzdata && \
    rm /etc/localtime && \
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && \
    dpkg-reconfigure -f noninteractive tzdata && \
    apt-get clean

# install cryprox uni dependecies
RUN echo "deb http://dist.yandex.ru/yandex-xenial unstable/amd64/" >> /etc/apt/sources.list.d/xenial-.yandex.list && \
    apt-get update -qq && \
    apt-get install -y python yandex-push-client=6.69.1 && \
    apt-get -y install yandex-solomon-agent-bin=1:15.3 && \
    apt-get -y install nginx=1.14.2-1.yandex.6 perl luarocks=2.2.0+dfsg-2 libssl-dev=1.0.2g-1ubuntu4.20 git lua-cjson=2.1.0+dfsg-2 gettext-base && \
    apt-get -y install --allow-unauthenticated yandex-porto && \
    git config --global url."https://github".insteadOf git://github && \
    luarocks install lua-resty-jwt 0.2.0 && \
    luarocks install lua-resty-string 0.09-0 && \
    luarocks install lua-resty-hmac v1.0-1 && \
    luarocks install lua-resty-woothee 1.8.0 && \
    apt-get -y purge luarocks git && \
    apt-get autoremove -y && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
# install filebeat https://www.elastic.co/guide/en/beats/filebeat/current/filebeat-installation.html
RUN curl -L -O http://s3.mds.yandex.net/sandbox-1526/3167557835/filebeat-7.4.2-linux-x86_64.tar.gz && \
    tar xvf filebeat-7.4.2-linux-x86_64.tar.gz && \
    rm filebeat-7.4.2-linux-x86_64.tar.gz && \
    ln -s /filebeat-7.4.2-linux-x86_64/filebeat /usr/bin/filebeat
