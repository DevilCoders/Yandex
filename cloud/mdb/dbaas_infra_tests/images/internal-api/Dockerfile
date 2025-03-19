FROM registry.yandex.net/ubuntu:bionic
LABEL image-cleanup=true

RUN echo "deb http://mirror.yandex.ru/ubuntu/ bionic main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb http://mirror.yandex.ru/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list

RUN rm -f /etc/apt/sources.list.d/common-* \
    /etc/apt/sources.list.d/yandex-bionic-* \
    /etc/apt/sources.list.d/testing.list \
    /etc/apt/sources.list.d/prestable.list \
    /etc/apt/sources.list.d/unstable.list

ENV container docker
ENV DEBIAN_FRONTEND noninteractive
RUN echo 'APT::Install-Recommends "0"; \n\
APT::Get::Assume-Yes "true"; \n\
APT::Get::force-yes "true"; \n\
APT::Install-Suggests "0";' > /etc/apt/apt.conf.d/01buildconfig && \
    echo 'deb http://dist.yandex.ru/mdb-bionic stable/all/' > /etc/apt/sources.list.d/mdb-bionic-stable.list && \
    echo 'deb http://dist.yandex.ru/mdb-bionic stable/$(ARCH)/' >> /etc/apt/sources.list.d/mdb-bionic-stable.list && \
    echo 'Package: *' > /etc/apt/preferences.d/00pgdg-pin-priority && \
    echo 'Pin: origin "apt.postgresql.org"' >> /etc/apt/preferences.d/00pgdg-pin-priority && \
    echo 'Pin-Priority: 1' >> /etc/apt/preferences.d/00pgdg-pin-priority && \
    echo 'Package: *' > /etc/apt/preferences.d/01dist-pin-priority && \
    echo 'Pin: origin "dist.yandex.ru"' >> /etc/apt/preferences.d/01dist-pin-priority && \
    echo 'Pin-Priority: 2' >> /etc/apt/preferences.d/01dist-pin-priority && \
    apt-get -o Acquire::AllowInsecureRepositories=true -o Acquire::AllowDowngradeToInsecureRepositories=true update && \
    apt-get -o APT::Get::AllowUnauthenticated=true -y install \
    iproute2 \
    nginx \
    nginx-common \
    supervisor

RUN ln -fs /usr/share/zoneinfo/Europe/Moscow /etc/localtime

RUN mkdir -p /opt/yandex && \
    mkdir -p /etc/supervisor/conf.d && \
    mkdir -p /config && \
    mkdir -m 750 -p /etc/nginx/ssl && chown root:www-data /etc/nginx/ssl && \
    ln -s /config/cert.pem /etc/nginx/ssl/cert.pem && \
    ln -s /config/key.pem /etc/nginx/ssl/key.pem && \
    mkdir -m 700 -p /root/.postgresql && \
    ln -s /config/CA.pem /root/.postgresql/root.crt

COPY docker_conf/supervisord.conf /etc/supervisor/supervisord.conf
COPY docker_conf/internal_api.conf /etc/supervisor/conf.d/internal_api.conf
COPY docker_conf/nginx_supervisor.conf /etc/supervisor/conf.d/nginx.conf
COPY docker_conf/nginx.conf /etc/nginx/nginx.conf

COPY uwsgi/etc/uwsgi.ini /opt/yandex/
COPY uwsgi/internal-api.wsgi /opt/yandex/

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
