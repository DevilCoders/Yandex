FROM ubuntu:focal-20210827

LABEL maintainer="Data Processing Team mdb-dataproc@yandex-team.ru"

#####
# Disable suggests/recommends
#####
RUN echo APT::Install-Recommends "0"\; > /etc/apt/apt.conf.d/10disableextras
RUN echo APT::Install-Suggests "0"\; >>  /etc/apt/apt.conf.d/10disableextras

#####
# Install common dependencies from packages.
#####
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Moscow
RUN     sed -i 's/http:\/\/archive.ubuntu.com\/ubuntu\//http:\/\/mirror.yandex.ru\/ubuntu\//' /etc/apt/sources.list && \
        /usr/bin/apt update -qq && /usr/bin/apt install -y  --no-install-recommends  \
        apt-transport-https \
        vim tree less wget git-core ccache \
        make clang cmake autoconf automake libtool \
        gcc g++ fuse liblzo2-dev libfuse-dev libcppunit-dev \
        libssl-dev libzip-dev sharutils pkg-config debhelper \
        devscripts build-essential dh-make libfuse2 libjansi-java \
        python2.7-dev libxml2-dev libxslt1-dev zlib1g-dev \
        libsqlite3-dev libldap2-dev libsasl2-dev libmariadbd-dev \
        python-setuptools libkrb5-dev asciidoc libzstd-dev \
        libyaml-dev libgmp3-dev libsnappy-dev libboost-regex-dev \
        xfslibs-dev libbz2-dev libreadline-dev zlib1g \
        libapr1 libapr1-dev libevent-dev libcurl4-openssl-dev \
        bison flex python-dev libffi-dev gnupg gnupg-agent curl \
        bats doxygen findbugs libbcprov-java \
        libsasl2-dev pinentry-curses pkg-config python python2.7 \
        python-pkg-resources python-setuptools rsync shellcheck \
        python3-pip python-apt \
        software-properties-common \
        xmlto sudo

######
# Install Google Protobuf 2.5.0 (2.6.0 ships with Xenial)
######
# hadolint ignore=DL3003
RUN /usr/bin/apt purge libprotobuf-dev  libprotobuf-lite17  libprotobuf17 && \
    mkdir -p /opt/protobuf-src \
    && curl -L -s -S \
      https://github.com/google/protobuf/releases/download/v2.5.0/protobuf-2.5.0.tar.gz \
      -o /opt/protobuf.tar.gz \
    && tar xzf /opt/protobuf.tar.gz --strip-components 1 -C /opt/protobuf-src \
    && cd /opt/protobuf-src \
    && ./configure --prefix=/usr \
    && make install \
    && cd /root \
    && rm -rf /opt/protobuf-src

ENV PROTOBUF_HOME /opt/protobuf
ENV PATH "${PATH}:/opt/protobuf/bin"

# Install aptly
COPY conf/apt-aptly-list /etc/apt/sources.list.d/aptly.list
COPY conf/aptly-config.json /root/.aptly.conf
RUN /usr/bin/curl https://www.aptly.info/pubkey.txt | /usr/bin/apt-key add - && \
        /usr/bin/apt-get update -qq && /usr/bin/apt-get install aptly && \
        sed -i -e 's/{{HOME}}/\/root/g' /root/.aptly.conf

# Install sdkman for java related stuff
ARG SDMAN_REPO="https://get.sdkman.io?rcupdate=false"
ENV SDKMAN_INIT "/usr/local/sdkman/bin/sdkman-init.sh"
ENV SDKMAN_DIR="/usr/local/sdkman"
RUN /usr/bin/apt-get install -y curl zip && \
        /usr/bin/curl -s "${SDMAN_REPO}" | /bin/bash && \
        /bin/bash -c "source ${SDKMAN_INIT} && sdk version && sdk update && sdk upgrade"

# install java related stuff
RUN /bin/bash -c "source ${SDKMAN_INIT} && \
        sdk install java 8.0.292.hs-adpt && \
        sdk install ant 1.10.10 && \
        sdk install maven 3.6.3 && \
        sdk install groovy 2.4.21 && \
        sdk install gradle 4.10.3 && \
        sdk install scala 2.12.13 && \
        sdk flush archives && sdk flush temp"
# install internal yandex artifactory cache to global maven config
COPY conf/mvn-settings.xml /usr/local/sdkman/candidates/maven/current/conf/settings.xml

COPY conf/dataproc-distro.sh /etc/profile.d/dataproc-distro.sh

# Install Node.JS for web UI framework
ARG NODE_VERSION="8.17.0"
ARG BRUNCH_VERSION="1.7.20"
COPY conf/.bowerrc /root/.bowerrc
RUN cd /usr/local && \
        /usr/bin/curl -s "https://nodejs.org/dist/v${NODE_VERSION}/node-v${NODE_VERSION}-linux-x64.tar.gz" | tar zxvf - && \
        /bin/ln -s /usr/local/node-v${NODE_VERSION}-linux-x64 /usr/local/node && \  
        /bin/ln /usr/local/node-v${NODE_VERSION}-linux-x64/bin/node /usr/local/bin/node && \
        /bin/ln /usr/local/node-v${NODE_VERSION}-linux-x64/bin/npm /usr/local/bin/npm && \
        /usr/local/node-v${NODE_VERSION}-linux-x64/bin/npm install -g bower@1.8.8 && \
        /usr/local/node-v${NODE_VERSION}-linux-x64/bin/npm install -g brunch@${BRUNCH_VERSION}

# # Install Yarn 1.12.1 for web UI framework
RUN /usr/bin/curl -s -S https://dl.yarnpkg.com/debian/pubkey.gpg | /usr/bin/apt-key add - \
    && echo 'deb https://dl.yarnpkg.com/debian/ stable main' > /etc/apt/sources.list.d/yarn.list \
    && apt-get -qq update \
    && apt-get install -y --no-install-recommends yarn=1.21.1-1

# Cleanup
RUN /usr/bin/apt-get clean && \
        cd /usr/src && rm -f *.deb *.zip *.tar.gz

CMD /bin/bash
