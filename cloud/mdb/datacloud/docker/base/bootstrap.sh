echo -e 'deb http://mirror.yandex.ru/ubuntu bionic main restricted universe multiverse\ndeb http://mirror.yandex.ru/ubuntu bionic-security main restricted universe multiverse\ndeb http://mirror.yandex.ru/ubuntu bionic-updates main restricted universe multiverse' > /etc/apt/sources.list
apt-get update -qq && apt-get upgrade -y && apt-get install -y \
        curl \
        dnsutils \
        gnupg \
        iputils-ping \
        less \
        locales \
        lsof \
        ltrace \
        mc \
        nano \
        net-tools \
        postgresql-client \
        redis-tools \
        strace \
        sudo \
        tcpdump \
        telnet \
        traceroute \
        util-linux \
        vim \
        wget \
        whois \

locale-gen en_US.UTF-8 && update-locale LANG=en_US.UTF-8
