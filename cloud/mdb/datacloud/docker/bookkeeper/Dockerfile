ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/bookkeeper /opt/yandex/billing-bookkeeper/
RUN useradd -ms /bin/bash billing-bookkeeper

USER billing-bookkeeper
WORKDIR /etc/yandex/billing-bookkeeper
# Omit CMD to enforce correct binary choice

