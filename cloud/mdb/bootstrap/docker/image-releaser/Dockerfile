ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/image-releaser /opt/yandex/image-releaser
RUN useradd -ms /bin/bash image-releaser

USER image-releaser
