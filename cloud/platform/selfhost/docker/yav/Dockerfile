FROM ubuntu:18.04

RUN echo "deb [trusted=yes] http://dist.yandex.ru/common stable/amd64/" >> /etc/apt/sources.list &&\
        apt-get update -qq ; apt-get install -y yandex-passport-vault-client=1.5.2 ; apt-get clean &&\
        rm -rf /etc/apt/sources.list /usr/bin/yav-deploy /var/lib/apt/lists
ENTRYPOINT [ "yav" ]
