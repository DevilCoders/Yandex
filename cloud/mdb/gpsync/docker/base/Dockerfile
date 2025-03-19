FROM ubuntu:bionic
ENV container docker
ENV DEBIAN_FRONTEND noninteractive
RUN echo 'APT::Install-Recommends "0"; \n\
APT::Get::Assume-Yes "true"; \n\
APT::Get::allow-downgrades "true"; \n\
APT::Install-Suggests "0";' > /etc/apt/apt.conf.d/01buildconfig && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list && \
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
        make \
        bind9-host \
        net-tools \
        iputils-ping \
        sudo \
        telnet \
        git \
        python3-dev \
        python3-pip \
        python3-setuptools \
        openssh-server \
        faketime \
        rsync \
        iptables && \
    pip3 install git+https://github.com/Supervisor/supervisor.git@4619168a4d820b37641a4719e211cf867bd7f49d && \
    rm -rf /var/run && \
    ln -s /dev/shm /var/run
COPY ./ /var/lib/dist
RUN mkdir /root/.ssh && \
    chmod 700 /root/.ssh && \
    cp /var/lib/dist/test_ssh_key.pub /root/.ssh/authorized_keys && \
    cp /var/lib/dist/test_ssh_key /root/.ssh/id_rsa && \
    mkdir -p /etc/supervisor/conf.d && \
    cp /var/lib/dist/docker/base/supervisord.conf /etc/supervisor/supervisord.conf && \
    cp /var/lib/dist/docker/base/ssh.conf /etc/supervisor/conf.d/ssh.conf
CMD ["/usr/local/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
