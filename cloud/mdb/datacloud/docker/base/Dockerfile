FROM cr.yandex/mirror/ubuntu:bionic

ENV DEBIAN_FRONTEND=noninteractive

COPY build/grpcurl /usr/bin/grpcurl
COPY build/dlv /usr/bin/dlv
COPY bootstrap.sh /bootstrap.sh

RUN bash /bootstrap.sh
