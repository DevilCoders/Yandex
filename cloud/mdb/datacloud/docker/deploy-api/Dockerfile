ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/mdb-deploy-api /opt/yandex/mdb-deploy-api/
#COPY build/mdb-deploy-cleaner /opt/yandex/mdb-deploy-api/
RUN useradd -ms /bin/bash deploy-api

USER deploy-api
WORKDIR /config
CMD ["/opt/yandex/mdb-deploy-api/mdb-deploy-api", "--scheme=http", "--host=localhost", "--port=12345"]
