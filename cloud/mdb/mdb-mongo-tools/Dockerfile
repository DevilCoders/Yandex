# vim:set ft=dockerfile:
FROM mdb-mongo-tools-base

ARG MONGODB_VERSION
ARG INSTALL_DIR="/opt/yandex/mdb-mongo-tools"
ARG CONF_DIR="/etc/yandex/mdb-mongo-tools"

RUN apt-get update -qq && \
    apt-get install -y apt-transport-https tzdata && \
    apt-get update -qq && \
    apt-key update && \
    apt-get download --allow-unauthenticated mongodb-org-server=$MONGODB_VERSION && \
    dpkg --unpack mongodb-org-server*.deb && \
    sed -i '/systemctl daemon-reload/d' /var/lib/dpkg/info/mongodb-org-server.postinst && \
    dpkg --configure mongodb-org-server && \
    apt-get install -yf && \
    apt-get install --allow-unauthenticated -y \
        mongodb-org-tools=$MONGODB_VERSION \
        mongodb-org-shell=$MONGODB_VERSION && \
    rm -rf /var/lib/apt/lists/* /var/cache/debconf /var/lib/mongodb/* && \
    apt-get clean

RUN mkdir -p ${INSTALL_DIR} ${CONF_DIR} && \
    ln --force -s /config/supervisor/conf.d/mongodb.conf /etc/supervisor/conf.d/mongodb.conf && \
    ln --force -s /config/mdb-mongo-tools.conf ${CONF_DIR}/mdb-mongo-tools.conf && \
    echo "secret" | base64 > /etc/mongod.keyfile && chmod 400 /etc/mongod.keyfile && chown mongodb /etc/mongod.keyfile


WORKDIR ${INSTALL_DIR}

ADD ./requirements.txt ${INSTALL_DIR}
RUN python3.6 -m pip install -i https://pypi.yandex-team.ru/simple -r requirements.txt

ADD . ${INSTALL_DIR}
RUN python3.6 -m pip install -i https://pypi.yandex-team.ru/simple -e .
