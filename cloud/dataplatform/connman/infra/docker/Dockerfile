# Base image for external cloud see at transfer_manager/ci/teamcity/build_docker_image
ARG BASE_IMAGE=registry.yandex.net/cdc/base
FROM $BASE_IMAGE:6a8c7f8e69666c7e191fd370378ea8f65496af9c

COPY connmanctl /usr/local/bin/connmanctl
COPY connmand /usr/local/bin/connmand
ARG CONFIG_FILE
COPY $CONFIG_FILE /etc/connman/config.yaml

EXPOSE 80 8787

CMD /usr/local/bin/connmand --config /etc/connman/config.yaml
