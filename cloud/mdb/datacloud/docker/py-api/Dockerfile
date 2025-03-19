ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest as builder

COPY build/uwsgi_internal_api /opt/yandex/internal-api-py/uwsgi_internal_api
RUN useradd -ms /bin/bash internal-api

USER internal-api
WORKDIR /opt/yandex/internal-api-py/
CMD ["/opt/yandex/internal-api-py/uwsgi_internal_api"]
