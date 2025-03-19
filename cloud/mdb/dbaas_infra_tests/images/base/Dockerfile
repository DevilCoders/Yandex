# vim:set ft=dockerfile:
FROM registry.yandex.net/ubuntu:bionic
LABEL image-cleanup=false

RUN echo "deb http://mirror.yandex.ru/ubuntu/ bionic main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list

ENV DEBIAN_FRONTEND noninteractive
ENV LANG en_US.utf8

# Configure apt and locale
COPY ./config/apt/ /var/lib/dist/config/apt
RUN cp --force --recursive --verbose \
    /var/lib/dist/config/apt/* /etc/apt/ && \
    apt-get -qq update && \
    apt-get -y install locales && \
    echo 'en_US.UTF-8 UTF-8' > /etc/locale.gen && \
    locale-gen && \
    apt-get -y install \
    supervisor \
    openssl git openssh-client \
    python3-pip \
    python3-setuptools \
    python3-dev \
    libffi-dev \
    build-essential \
    netcat-openbsd \
    iputils-ping \
    net-tools \
    dnsutils \
    nginx

# Create dirs shared by mocks.
RUN mkdir -p /opt/yandex /code /config

# Install python packages shared by mocks.
RUN mkdir /tmp/base_image
COPY ./src /tmp/base_image/src
# Workaround for https://github.com/moby/moby/issues/37965
RUN true
COPY ./requirements.txt /tmp/base_image/
RUN cd /tmp/base_image/ && \
    pip3 install --upgrade pip==9.0.3 && \
    hash -r pip && \
    pip3 install \
        -r requirements.txt \
        src/server_mock/ && \
    rm -rf /tmp/base_image

# This config directory is never mounted, and thus should be copied.
COPY ./config/supervisor /etc/supervisor

# Almost all mocks have custom nginx conf.
RUN ln -fs /config/nginx.conf /etc/nginx/nginx.conf

EXPOSE 80 443

CMD ["supervisord", "-c", "/etc/supervisor/supervisord.conf"]
