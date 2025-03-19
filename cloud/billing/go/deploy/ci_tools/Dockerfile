FROM cr.yandex/mirror/ubuntu:20.04

RUN echo "" >/etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ focal main restricted">>/etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ focal-updates main restricted">>/etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ focal universe">>/etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ focal-updates universe">>/etc/apt/sources.list && \
    apt-get update && apt-get install -y gnupg curl less jq

RUN mkdir -p /usr/local/share/ca-certificates/ &&\
    curl https://crls.yandex.net/YandexInternalRootCA.crt -o /usr/local/share/ca-certificates/YandexInternalRootCA.crt &&\
    update-ca-certificates

RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 7FCD11186050CD1A  &&\
    # curl -fsSL https://apt.releases.hashicorp.com/gpg | apt-key add - &&\
    # echo "deb [arch=amd64] https://apt.releases.hashicorp.com focal main">>/etc/apt/sources.list &&\
    apt-get update -y && \
    # apt-get -y install python3 python-is-python3 terraform
    apt-get -y install python3 python-is-python3 unzip

RUN curl https://hashicorp-releases.website.yandexcloud.net/terraform/1.1.6/terraform_1.1.6_linux_amd64.zip -o /tmp/terraform.zip && unzip /tmp/terraform.zip -d /usr/bin/ && rm /tmp/terraform.zip &&\
    curl -L https://github.com/gruntwork-io/terragrunt/releases/download/v0.36.3/terragrunt_linux_amd64 -o /usr/bin/terragrunt && chmod a+x /usr/bin/terragrunt && \
    curl https://storage.cloud-preprod.yandex.net/skm/linux/skm -o /usr/bin/skm && chmod a+x /usr/bin/skm

COPY files/providers.tf /vendor/

RUN mkdir -p /root/.terraform.d/plugin-cache /usr/share/terraform/providers &&\
    cd /vendor && terraform providers mirror /usr/share/terraform/providers && \
    cp -r /usr/share/terraform/providers/terraform-registry.storage.yandexcloud.net /usr/share/terraform/providers/registry.terraform.io

COPY files/terraformrc /root/.terraformrc

COPY yt-oauth/yt-oauth /usr/bin/yt-oauth
COPY juggler_cli/juggler_cli /usr/bin/juggler_cli

ENV USE_ENV_TOOLS=1
