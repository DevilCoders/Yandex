ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

RUN useradd -ms /bin/bash vpc-worker
USER vpc-worker
WORKDIR /etc/yandex/vpc-worker
CMD ["/opt/yandex/vpc-worker/mdb-vpc-worker"]
COPY build/mdb-vpc-worker /opt/yandex/vpc-worker/
