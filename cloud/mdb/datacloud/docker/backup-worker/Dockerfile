ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/backup-worker /opt/yandex/backup-worker/
RUN useradd -ms /bin/bash backup-worker

USER backup-worker
WORKDIR /etc/yandex/backup-worker
CMD ["/opt/yandex/backup-worker/backup-worker", "--config-path", "/etc/yandex/backup-worker/"]
