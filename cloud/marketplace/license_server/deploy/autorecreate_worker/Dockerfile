FROM cr.yandex/mirror/ubuntu:20.04

COPY cmd/autorecreate_worker /usr/bin/
COPY version.txt /version.txt

ENTRYPOINT [ "/usr/bin/autorecreate_worker", "-c", "/configs/*.yaml"]
