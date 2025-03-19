ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

RUN apt install -y tzdata

COPY build/grpc_health_probe /grpc_health_probe

COPY build/datacloud-internal-api /opt/yandex/datacloud-internal-api/
COPY build/mdb-internal-api /opt/yandex/mdb-internal-api/
RUN useradd -ms /bin/bash internal-api

USER internal-api
