ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/mdb-secrets /opt/yandex/mdb-secrets/
RUN useradd -ms /bin/bash mdb-secrets

USER mdb-secrets
WORKDIR /etc/yandex/mdb-secrets
CMD ["/opt/yandex/mdb-secrets/mdb-secrets", "--scheme=http", "--port=12345", "--host=127.0.0.1"]
