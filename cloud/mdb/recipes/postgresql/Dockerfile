# vim:set ft=dockerfile:
FROM ubuntu:precise

RUN echo "deb http://old-releases.ubuntu.com/ubuntu/ precise main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://old-releases.ubuntu.com/ubuntu/ precise-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://old-releases.ubuntu.com/ubuntu/ precise-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://old-releases.ubuntu.com/ubuntu/ precise-security main restricted universe multiverse" >> /etc/apt/sources.list

RUN apt-get -o Acquire::AllowInsecureRepositories=true -o Acquire::AllowDowngradeToInsecureRepositories=true update \
    && apt-get -o APT::Get::AllowUnauthenticated=true install -y postgresql-common \
        sudo \
        build-essential \
        libreadline-dev \
        zlib1g-dev \
        flex \
        bison \
        libxml2-dev \
        libxslt-dev \
        libssl-dev \
        libxml2-utils \
        xsltproc \
        uuid-dev \
        libicu-dev \
        git \
        pkg-config \
        wget

# https://www.openssl.org/blog/blog/2021/09/13/LetsEncryptRootCertExpire/
RUN rm -f /usr/share/ca-certificates/mozilla/DST_Root_CA_X3.crt \
    && update-ca-certificates

RUN git config --global user.email "mdb@yandex-team.ru"
RUN git config --global user.name "Recipe builder"

RUN mkdir /recipe
COPY build.sh /
WORKDIR recipe
CMD "/build.sh"
