FROM cr.yandex/yc-internal/ubuntu-build-dep:manual

WORKDIR /

COPY filestore-vhost /usr/bin/filestore-vhost

RUN useradd --create-home yc-nfs
USER yc-nfs

CMD ["/bin/true"]
