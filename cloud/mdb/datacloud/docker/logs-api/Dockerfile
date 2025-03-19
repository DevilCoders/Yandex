ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

RUN apt install -y tzdata

COPY build/grpc_health_probe /grpc_health_probe

COPY build/logs-api /opt/yandex/logs-api/
RUN useradd -ms /bin/bash logs-api

USER logs-api
WORKDIR /etc/yandex/logs-api
CMD ["/opt/yandex/logs-api/logs-api"]
