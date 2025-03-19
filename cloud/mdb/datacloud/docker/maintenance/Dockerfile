ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY /build/mdb-maintenance-sync /opt/yandex/mdb-maintenance-sync
COPY /configs /etc/yandex/mdb-maintenance/configs/

WORKDIR /etc/yandex/mdb-maintenance/

CMD ["/opt/yandex/mdb-maintenance-sync/mdb-maintenance-sync", "--config-path=/etc/yandex/mdb-maintenance/"]
