FROM cr.yandex/mirror/ubuntu:20.04

COPY cmd/license-check /usr/bin/
COPY version.txt /version.txt

ENTRYPOINT [ "/usr/bin/license-check", "-c", "/configs/*.yaml"]
