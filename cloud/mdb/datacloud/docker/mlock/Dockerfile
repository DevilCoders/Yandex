ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

RUN useradd -ms /bin/bash mlock
COPY build/grpc_health_probe /grpc_health_probe
COPY build/mdb-mlock /opt/yandex/mdb-mlock/
COPY build/mdb-mlock-cli /usr/local/bin/mlock

USER mlock
WORKDIR /etc/yandex/mdb-mlock
CMD ["/opt/yandex/mdb-mlock/mdb-mlock", "/etc/yandex/mdb-mlock/mlock.yaml"]
