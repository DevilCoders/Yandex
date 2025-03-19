echo -e 'deb http://mirror.yandex.ru/ubuntu focal main restricted universe multiverse\ndeb http://mirror.yandex.ru/ubuntu focal-security main restricted universe multiverse\ndeb http://mirror.yandex.ru/ubuntu focal-updates main restricted universe multiverse' > /etc/apt/sources.list
apt-get update -qq && apt-get upgrade -y && apt-get install -y \
        curl \
        dnsutils \
        git \
        gnupg \
        hub \
        iputils-ping \
        less \
        locales \
        lsof \
        ltrace \
        mc \
        nano \
        net-tools \
        python3 \
        python3-retrying \
        strace \
        sudo \
        tcpdump \
        telnet \
        traceroute \
        util-linux \
        vim \
        wget \
        whois \

wget https://github.com/mikefarah/yq/releases/download/v4.16.2/yq_linux_amd64 -O /yq && sudo chmod +x /yq

locale-gen en_US.UTF-8 && update-locale LANG=en_US.UTF-8
