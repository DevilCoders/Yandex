FROM yaubuntu
ARG pg_version=12
ENV PG_VERSION ${pg_version}
ENV LEIN_ROOT 1
ENV container docker
ENV DEBIAN_FRONTEND noninteractive

RUN echo 'APT::Install-Recommends "0"; \n\
APT::Get::Assume-Yes "true"; \n\
APT::Get::force-yes "true"; \n\
APT::Install-Suggests "0";' > /etc/apt/apt.conf.d/01buildconfig && \
    apt-get update && apt-get install wget gnupg ca-certificates locales && \
    locale-gen en_US.UTF-8 && \
    echo "deb http://dist.yandex.ru/mdb-bionic stable/all/" > /etc/apt/sources.list.d/mdb-bionic-stable.list && \
    echo "deb http://dist.yandex.ru/mdb-bionic stable/\$(ARCH)/" >> /etc/apt/sources.list.d/mdb-bionic-stable.list && \
    apt-get -qq -o Acquire::AllowInsecureRepositories=true \
    -o Acquire::AllowDowngradeToInsecureRepositories=true update && \
    apt-get -o APT::Get::AllowUnauthenticated=true -y install yandex-archive-keyring && \
    apt-get update && \
    apt-get install \
        openjdk-11-jre-headless \
        less \
        bind9-host \
        net-tools \
        iputils-ping \
        sudo \
        telnet \
        git \
        pgbouncer \
        python3-dev \
        python3-pip \
        python3-venv \
        python3-setuptools \
        libpq-dev \
        gcc \
        faketime \
        rsync \
        build-essential \
        iptables && \
    rm -rf /var/run && \
    ln -s /dev/shm /var/run

COPY ./ /var/lib/dist

RUN apt-get -qq update && apt-get install libjna-java gnuplot wget && \
    wget https://raw.githubusercontent.com/technomancy/leiningen/stable/bin/lein -O /usr/bin/lein && \
    chmod +x /usr/bin/lein && \
    cp -r /var/lib/dist/jepsen /root/ && \
    cd /root/jepsen && \
    lein install && \
    lein deps

COPY test_ssh_key /root/.ssh/id_rsa

RUN apt-get update -qq \
    && apt-get install -y \
    postgresql-client-${PG_VERSION} \
    python-psycopg2

COPY setup.sh /setup.sh
