ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/grpc_health_probe /grpc_health_probe

COPY build/mdb-pillar-secrets /opt/yandex/mdb-pillar-secrets/
RUN useradd -ms /bin/bash pillar-secrets

USER pillar-secrets
WORKDIR /etc/yandex/mdb-pillar-secrets
CMD ["/opt/yandex/mdb-pillar-secrets/mdb-pillar-secrets"]
