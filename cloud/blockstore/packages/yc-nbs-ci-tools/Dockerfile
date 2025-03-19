FROM registry.yandex.net/ubuntu:xenial

WORKDIR /

COPY checkpoint-validator /usr/bin/checkpoint-validator
COPY verify-test /usr/bin/verify-test
COPY yc-nbs-ci-checkpoint-validation-test /usr/bin/yc-nbs-ci-checkpoint-validation-test
COPY yc-nbs-ci-check-nrd-disk-emptiness-test /usr/bin/yc-nbs-ci-check-nrd-disk-emptiness-test
COPY yc-nbs-ci-corruption-test-suite /usr/bin/yc-nbs-ci-corruption-test-suite
COPY yc-nbs-ci-deploy-packages /usr/bin/yc-nbs-ci-deploy-packages
COPY yc-nbs-ci-fio-performance-test-suite /usr/bin/yc-nbs-ci-fio-performance-test-suite
COPY yc-nbs-ci-migration-test /usr/bin/yc-nbs-ci-migration-test
COPY yc-nbs-ci-run-ya-make /usr/bin/yc-nbs-ci-run-ya-make
COPY yc-nbs-ci-run-ya-package /usr/bin/yc-nbs-ci-run-ya-package
COPY yc-nbs-ci-sign-packages /usr/bin/yc-nbs-ci-sign-packages
COPY yc-nbs-ci-worst-case-read-performance-test-suite /usr/bin/yc-nbs-ci-worst-case-read-performance-test-suite
COPY acceptance-test /usr/bin/acceptance-test
COPY yc-disk-manager-ci-acceptance-test /usr/bin/yc-disk-manager-ci-acceptance-test
COPY yc-nfs-ci-build-arcadia-test /usr/bin/yc-nfs-ci-build-arcadia-test

RUN apt-get -qq update
RUN apt-get -y install curl jq python ssh subversion tcpdump sudo iputils-ping

USER root

RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

RUN curl https://s3.mds.yandex.net/mcdev/ycp/install.sh | bash
ENV PATH="/root/ycp/bin/:${PATH}"
COPY /ycp-config.yaml /root/.config/ycp/config.yaml

RUN mkdir -p /root/.ssh
RUN echo -e "Host *\n   StrictHostKeyChecking no\n   UserKnownHostsFile /dev/null\n   LogLevel ERROR\n PreferredAuthentications publickey" > /root/.ssh/config

CMD ["/bin/true"]
