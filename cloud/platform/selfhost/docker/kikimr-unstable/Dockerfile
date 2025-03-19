FROM registry.yandex.net/ubuntu:precise

ADD deb /etc/apt/sources.list.d/
COPY bin/kikimr.sh /bin/kikimr.sh

ARG YDB_VERSION=4498870-branches-kikimr-stable-19-2-test-version
ARG YDB_RECIPE_VERSION=3794804.trunk.268191309

RUN \
    chmod a+x /bin/kikimr.sh && \
    mkdir -p /Berkanavt && \
    apt-get update -qq && \
    apt-get install --force-yes -y yandex-search-kikimr-kikimr-bin=${YDB_VERSION} yandex-search-kikimr-yql-udfs-kikimr=${YDB_VERSION} yandex-search-kikimr-ydb-recipe-bin=${YDB_RECIPE_VERSION} openssh-server openssh-client rsync psmisc netcat-openbsd lsof net-tools socat && \
    mkdir -p /var/run/sshd && \
    ssh-keygen -f /root/.ssh/id_rsa -trsa -N "" && \
    cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys && \
    chmod og-wx ~/.ssh/authorized_keys && \
    apt-get clean

HEALTHCHECK --interval=2s --timeout=1s --retries=5 CMD /usr/bin/kikimr db schema ls / || exit 1

EXPOSE 2135

CMD ["sh", "-c", "/bin/kikimr.sh"]
