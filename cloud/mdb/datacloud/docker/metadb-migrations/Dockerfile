ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest as builder

COPY build/dbaas_metadb /opt/datacloud/dbaas_metadb
COPY build/pgmigrate /usr/local/bin/pgmigrate
COPY build/pgmigrator /usr/local/bin/pgmigrator
COPY build/create_users /usr/local/bin/create_users
COPY build/populate_table /usr/local/bin/populate_table
COPY build/vr_gen2 /usr/local/bin/vr_gen2
COPY build/vr_gen /usr/local/bin/vr_gen
COPY migrate.sh /usr/local/bin/migrate
RUN chmod +x /usr/local/bin/migrate
COPY resources /opt/datacloud/resources

WORKDIR /opt/datacloud/
