FROM cr.yandex/yc-internal/ubuntu-build-dep:manual

WORKDIR /

COPY filestore-server /usr/bin/filestore-server

RUN useradd --create-home yc-nfs
USER yc-nfs

CMD ["/bin/true"]
