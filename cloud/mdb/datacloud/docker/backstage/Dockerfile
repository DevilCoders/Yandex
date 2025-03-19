ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/gunicorn /opt/doublecloud/backstage/
COPY build/manage /opt/doublecloud/backstage/
COPY build/static /opt/doublecloud/backstage/static
RUN useradd -ms /bin/bash backstage

USER backstage
WORKDIR /opt/doublecloud/backstage
CMD ["/opt/doublecloud/backstage/gunicorn"]
