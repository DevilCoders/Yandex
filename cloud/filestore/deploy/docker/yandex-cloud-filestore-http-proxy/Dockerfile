FROM cr.yandex/yc-internal/ubuntu-build-dep:manual

WORKDIR /

COPY filestore-http-proxy /usr/bin/filestore-http-proxy

RUN useradd --create-home yc-nfs
USER yc-nfs

CMD ["/bin/true"]
