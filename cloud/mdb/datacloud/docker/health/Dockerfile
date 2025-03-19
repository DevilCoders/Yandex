ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/mdb-health /opt/yandex/mdb-health/
RUN useradd -ms /bin/bash health

USER health
WORKDIR /config
CMD ["/opt/yandex/mdb-health/mdb-health", "--scheme=http", "--host=localhost", "--port=12345"]
