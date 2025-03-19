ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/e2e-tests /opt/yandex/e2e-tests/
RUN useradd -ms /bin/bash dbaas-e2e

WORKDIR /etc/yandex/e2e-tests
