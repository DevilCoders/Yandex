ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/mdb-pillar-config /opt/yandex/mdb-pillar-config/
RUN useradd -ms /bin/bash pillar-config

USER pillar-config
WORKDIR /etc/yandex/mdb-pillar-config
CMD ["/opt/yandex/mdb-pillar-config/mdb-pillar-config"]
