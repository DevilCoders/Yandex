FROM cr.yandex/yc-internal/ubuntu-build-dep:manual

WORKDIR /

COPY filestore-nfs /usr/bin/filestore-nfs

RUN useradd --create-home yc-nfs
USER yc-nfs

CMD ["/bin/true"]
