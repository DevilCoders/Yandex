ARG VERSION=latest
ARG FROM=dataproc/distro-toolchain-ubuntu-2004:${VERSION}
FROM ${FROM}

COPY toolchain/conf/dataproc-gpg.sh /etc/profile.d/dataproc-gpg.sh
COPY toolchain/conf/s3_credentials.sh /etc/profile.d/s3_credentials.sh
RUN mkdir -p /ws

ARG UNAME=dataproc
ARG GNAME=dpt_virtual_robots_1561
ARG UID=1000
ARG GID=1000
RUN groupadd -g $GID -o $GNAME && \
    useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME && \
    chown -R ${UNAME}:${GNAME} /ws && \
    adduser ${UNAME} sudo && \
    echo "${UNAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
USER $UNAME
COPY toolchain/conf/aptly-config.json /home/${UNAME}/.aptly.conf
RUN mkdir -p /home/${UNAME}/.ssh && \
    chmod 700 /home/${UNAME}/.ssh && \
    sed -i -e "s/{{HOME}}/\/home\/${UNAME}/g" /home/${UNAME}/.aptly.conf && \
    chown -R ${UNAME}:${GROUP} /ws
