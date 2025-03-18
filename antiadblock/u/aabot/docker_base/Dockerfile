FROM ubuntu:xenial

COPY ./etc /etc


# install yandex certs/keys and tools
RUN echo "deb http://dist.yandex.ru/yandex-xenial unstable/amd64/" >> /etc/apt/sources.list.d/xenial-.yandex.list
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
