FROM registry.yandex.net/antiadb/ubuntu-xenial:6-135d40

COPY . /
WORKDIR /etc/nginx/

RUN apt update -qq && \
    apt -y install nginx=1.14.2-1.yandex.6 perl && \
    apt autoremove -y && \
    apt clean && \
    rm -rf /var/lib/apt/lists/*
