ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest as builder

COPY build/create_users /usr/local/bin/create_users
COPY migrate.sh /usr/local/bin/migrate
RUN chmod +x /usr/local/bin/migrate

WORKDIR /opt/datacloud/
