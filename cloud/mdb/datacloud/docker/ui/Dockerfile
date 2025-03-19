ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

COPY build/http /opt/yandex/ui/
COPY build/manage /opt/yandex/ui/
RUN useradd -ms /bin/bash ui

RUN mkdir -p /var/ui/
RUN chown ui /var/ui/

USER ui
WORKDIR /opt/yandex/ui/

RUN mkdir -p /var/ui/site_static
RUN mkdir -p /var/ui/media
RUN mkdir -p /var/ui/static
RUN mkdir -p /var/ui/logs
RUN mkdir -p /var/ui/templates

RUN UI_SECRET_KEY=test UI_LOAD_CONFIG_FROM_PKG="cloud.mdb.ui.internal,config.ini" /opt/yandex/ui/manage migrate
RUN UI_SECRET_KEY=test UI_LOAD_CONFIG_FROM_PKG="cloud.mdb.ui.internal,config.ini" /opt/yandex/ui/manage collectstatic --noinput -v 3
