ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/backup-cli /opt/yandex/backup-cli/
RUN useradd -ms /bin/bash backup-cli

USER backup-cli
WORKDIR /etc/yandex/backup-cli
CMD ["/opt/yandex/backup-cli/backup-cli", "--config-path", "/etc/yandex/backup-cli/"]
