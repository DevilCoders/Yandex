FROM ubuntu:xenial

MAINTAINER https://a.yandex-team.ru/arc/trunk/arcadia/cloud/platform/selfhost/docker

ADD conf/*.list /etc/apt/sources.list.d/

RUN sed -i "s/archive.ubuntu.com/mirror.yandex.ru/g" /etc/apt/sources.list

RUN apt-get update -qq && \
    apt-get dist-upgrade --force-yes -y && \
    apt-get install --force-yes -y yandex-internal-root-ca yandex-ca-certs yandex-archive-keyring yandex-conf-repo-prestable yandex-conf-repo-testing yandex-conf-repo-unstable less wget curl vim lsof htop iotop tcpdump netcat net-tools&& \
    apt-get clean && \
    ln -sf bash /bin/sh

