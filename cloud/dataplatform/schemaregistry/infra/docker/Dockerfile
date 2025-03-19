ARG BASE_IMAGE=registry.yandex.net/cdc/base
FROM $BASE_IMAGE:6a8c7f8e69666c7e191fd370378ea8f65496af9c

COPY schemaregistry /usr/local/bin/schemaregistry
ARG CONFIG_FILE
COPY $CONFIG_FILE /etc/schemaregistry/config.yaml

EXPOSE 80 8787

CMD /usr/local/bin/schemaregistry --config /etc/schemaregistry/config.yaml
