FROM cr.yandex/mirror/io/docker/python:3.7.9-slim

COPY requirements*.txt /tmp/

WORKDIR /root

RUN apt-get update
RUN apt-get install -y vim gcc python2.7 python2.7-dev python-pip libffi-dev curl jq
RUN mkdir -p project/ && mkdir -p .pip/
RUN pip3 install -r /tmp/requirements3.txt
RUN pip2 install -r /tmp/requirements2.txt

RUN rm -rf /var/lib/apt/lists/* /tmp/requirements*.txt

# ycp is required for issuing IAM tokens in solomon/*/sync.sh
RUN /bin/bash -cex 'curl https://s3.mds.yandex.net/mcdev/ycp/install.sh | bash'
COPY ycp.config.yaml /root/.config/ycp/config.yaml

# Install YandexInternalRootCA — it's required for ycp. See https://wiki.yandex-team.ru/security/ssl/sslclientfix/#ubuntu
RUN mkdir -p /usr/local/share/ca-certificates/ && \
    curl https://crls.yandex.net/YandexInternalRootCA.crt > /usr/local/share/ca-certificates/YandexInternalRootCA.crt && \
    update-ca-certificates

WORKDIR /root/project
