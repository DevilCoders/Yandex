FROM cr.yandex/mirror/ubuntu:20.04

COPY cmd/license_server /usr/bin/
COPY dev/lsc /usr/bin/
COPY version.txt /version.txt

ENTRYPOINT [ "/usr/bin/license_server", "-c", "/configs/*.yaml"]
