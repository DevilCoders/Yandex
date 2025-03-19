# vim:set ft=dockerfile:
FROM registry.yandex.net/ubuntu:bionic

ENV DEBIAN_FRONTEND noninteractive
ENV LANG en_US.utf8

# Configure apt and locale
COPY ./config/apt/ /var/lib/dist/config/apt
RUN cp --force --recursive --verbose /var/lib/dist/config/apt/* /etc/apt/

RUN apt-get clean && apt-get update && apt-get install -y locales && \
    echo 'en_US.UTF-8 UTF-8' > /etc/locale.gen && \
    locale-gen && \
    apt-get -qq update && \
    \
    apt-get -y install \
    supervisor=3.3.1-1yandex \
    openssl git openssh-client openssh-server \
    python3.6 \
    python3.6-venv \
    python3-pip \
    python3-setuptools \
    netcat-openbsd iputils-ping net-tools dnsutils \
    apt-transport-https tzdata

RUN mkdir -p /opt/yandex /code /config

RUN python3.6 -m pip install --upgrade pip

RUN echo "root:root" | chpasswd
RUN sed -i -e '/PermitRootLogin/ s/ .*/ yes/' /etc/ssh/sshd_config
RUN mkdir /var/run/sshd

COPY ./config/supervisor /etc/supervisor

CMD ["supervisord", "-c", "/etc/supervisor/supervisord.conf"]
