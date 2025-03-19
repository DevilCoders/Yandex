ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/mdb-dns /opt/yandex/mdb-dns/
RUN useradd -ms /bin/bash dns

USER dns
WORKDIR /config
CMD ["/opt/yandex/mdb-dns/mdb-dns", "--scheme=http", "--host=localhost", "--port=26011"]
