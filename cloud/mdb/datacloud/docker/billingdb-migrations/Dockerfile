ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/billingdb /opt/yandex/billingdb
COPY build/pgmigrate /usr/local/bin/pgmigrate
COPY build/create_users /usr/local/bin/create_users
COPY migrate.sh /usr/local/bin/migrate
RUN chmod +x /usr/local/bin/migrate

WORKDIR /opt/yandex/
