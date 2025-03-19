ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/sender /opt/yandex/billing-sender
RUN useradd -ms /bin/bash billing-sender

USER billing-sender
WORKDIR /etc/yandex/billing-sender
# Omit CMD to enforce correct binary choice
