ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY /build/mdb-reaper /opt/yandex/mdb-reaper

WORKDIR /etc/yandex/mdb-reaper/

CMD ["/opt/yandex/mdb-reaper/mdb-reaper"]
