FROM ubuntu:xenial
ADD conf/*.list /etc/apt/sources.list.d/
RUN sed -i "s/archive.ubuntu.com/mirror.yandex.ru/g" /etc/apt/sources.list
RUN apt-get update -qq && \
    apt-get dist-upgrade --force-yes -y && \
    apt-get install --force-yes -y yandex-internal-root-ca yandex-ca-certs yandex-archive-keyring yandex-conf-repo-prestable yandex-conf-repo-testing yandex-conf-repo-unstable && \
    apt-get clean && \
    ln -sf bash /bin/sh
RUN apt-get install --force-yes -y libapr1 libapr1-dev yandex-openjdk11=11.0.5-tzdata2019b
