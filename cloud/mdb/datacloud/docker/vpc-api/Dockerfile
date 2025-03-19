ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

RUN useradd -ms /bin/bash vpc-api
USER vpc-api
WORKDIR /etc/yandex/vpc-api
CMD ["/opt/yandex/vpc-api/mdb-vpc-api"]
COPY build/grpc_health_probe /grpc_health_probe
COPY build/mdb-vpc-api /opt/yandex/vpc-api/
