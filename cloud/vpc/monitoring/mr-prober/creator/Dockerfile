ARG PYTHON_VERSION=3.10
ARG PYTHON_IMAGE_VERSION=$PYTHON_VERSION

FROM cr.yandex/mirror/python:$PYTHON_IMAGE_VERSION

WORKDIR /mr_prober

RUN apt-get update && apt-get install -q -y wget unzip gettext-base jq curl

RUN pip install --upgrade pip

# Install terraform 1.1.5
RUN wget --quiet https://hashicorp-releases.website.yandexcloud.net/terraform/1.1.5/terraform_1.1.5_linux_amd64.zip
RUN unzip terraform_1.1.5_linux_amd64.zip && rm terraform_1.1.5_linux_amd64.zip
RUN mv terraform /usr/bin/terraform

# Install terraform providers specified in creator/files/terraform_providers.tf.json
COPY creator/files/terraform_providers.tf.json terraform_providers.tf.json
COPY creator/files/mirror_providers.sh mirror_providers.sh
RUN /bin/bash -ex /mr_prober/mirror_providers.sh

COPY creator/files/.terraformrc /root/.terraformrc

# Copy ycp config to container
COPY creator/files/ycp.config.yaml /root/.config/ycp/config.yaml

# Install yc. See https://cloud.yandex.ru/docs/cli/operations/install-cli
RUN wget --quiet https://storage.yandexcloud.net/yandexcloud-yc/install.sh -O - | bash -s -- -a

# Install skm for secrets encrypting. See https://wiki.yandex-team.ru/cloud/devel/platform-team/infra/skm/
RUN wget --quiet https://storage.cloud-preprod.yandex.net/skm/linux/skm -O /usr/bin/skm
RUN chmod +x /usr/bin/skm

# Install YandexInternalRootCA — i.e. it's needed for yav client. See https://wiki.yandex-team.ru/security/ssl/sslclientfix/#ubuntu
RUN mkdir -p /usr/local/share/ca-certificates/ && \
    wget --quiet https://crls.yandex.net/YandexInternalRootCA.crt -O /usr/local/share/ca-certificates/YandexInternalRootCA.crt && \
    update-ca-certificates

COPY requirements.production.txt requirements.txt ./

RUN pip install -r requirements.production.txt --no-cache-dir

COPY . .

ENV PYTHONPATH=/mr_prober

CMD [ "python", "/mr_prober/creator/main.py" ]
