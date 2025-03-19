FROM ubuntu:xenial-20210114

COPY yandex-cloud-secure.list /etc/apt/sources.list.d/

ENV YANDEX_ARCHIVE_KEYRING_VERSION="2016.06.17-1"
RUN apt-get update \
    && apt-get install -y --allow-unauthenticated yandex-archive-keyring=${YANDEX_ARCHIVE_KEYRING_VERSION} 
RUN apt-get install -y curl
