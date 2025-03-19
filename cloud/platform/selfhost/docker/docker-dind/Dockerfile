FROM docker:18.09.3-dind

RUN \
    wget https://crls.yandex.net/YandexInternalRootCA.crt -O /usr/local/share/ca-certificates/YandexInternalRootCA.crt &&\
    update-ca-certificates