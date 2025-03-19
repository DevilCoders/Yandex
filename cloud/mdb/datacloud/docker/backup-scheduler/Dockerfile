ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/backup-scheduler /opt/yandex/backup-scheduler/
RUN useradd -ms /bin/bash backup-scheduler

USER backup-scheduler
WORKDIR /etc/yandex/backup-scheduler
CMD ["/opt/yandex/backup-scheduler/backup-scheduler", "--config-path", "/etc/yandex/backup-scheduler/"]
