FROM ubuntu:xenial

ADD repositories/*.list /etc/apt/sources.list.d/

RUN apt-get update -qq && \
    apt-get dist-upgrade -y --allow-unauthenticated && \
    apt-get install -y --allow-unauthenticated \
        yandex-internal-root-ca \
        yandex-ca-certs \
        yandex-archive-keyring \
        yandex-conf-repo-prestable \
        yandex-conf-repo-testing \
        yandex-conf-repo-unstable \
        less \
        wget \
        curl \
        vim \
        lsof \
        htop \
        iotop \
        tcpdump \
        netcat \
        net-tools \
        libapr1 \
        libapr1-dev \
        yandex-openjdk8 && \
    apt-get clean && \
    ln -sf bash /bin/sh

COPY ./keys /var/tmp/cloud_keys/

COPY ./nirvana-job-processor-1.0-SNAPSHOT-runnable.jar /usr/app/
WORKDIR /usr/app
EXPOSE 6000
EXPOSE 8000

ENTRYPOINT ["java", "-jar", "-agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=8000", "nirvana-job-processor-1.0-SNAPSHOT-runnable.jar"]
