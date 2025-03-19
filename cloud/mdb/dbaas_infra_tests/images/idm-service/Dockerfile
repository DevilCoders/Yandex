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

RUN echo 'APT::Install-Recommends "0"; \n\
APT::Get::Assume-Yes "true"; \n\
APT::Get::force-yes "true"; \n\
APT::Install-Suggests "0";' > /etc/apt/apt.conf.d/01buildconfig && \
    echo 'deb http://dist.yandex.ru/mdb-bionic stable/all/' > /etc/apt/sources.list.d/mdb-bionic-stable.list && \
    echo 'deb http://dist.yandex.ru/mdb-bionic stable/$(ARCH)/' >> /etc/apt/sources.list.d/mdb-bionic-stable.list && \
    echo 'Package: *' > /etc/apt/preferences.d/01dist-pin-priority && \
    echo 'Pin: origin "dist.yandex.ru"' >> /etc/apt/preferences.d/01dist-pin-priority && \
    echo 'Pin-Priority: 2' >> /etc/apt/preferences.d/01dist-pin-priority && \
    apt-get -o Acquire::AllowInsecureRepositories=true -o Acquire::AllowDowngradeToInsecureRepositories=true update && \
    apt-get -o APT::Get::AllowUnauthenticated=true -y install \
        nginx \
        nginx-common \
        supervisor

COPY ./ /var/lib/dist
RUN mkdir -p /opt/yandex/mdb-idm-service && \
    mkdir -p /config && \
    mkdir -p /etc/supervisor/conf.d && \
    cp /var/lib/dist/docker/supervisord.conf /etc/supervisor/supervisord.conf && \
    cp /var/lib/dist/docker/idm_service.conf /etc/supervisor/conf.d/idm_service.conf && \
    cp /var/lib/dist/docker/nginx_supervisor.conf /etc/supervisor/conf.d/nginx.conf && \
    cp /var/lib/dist/docker/nginx.conf /etc/nginx/nginx.conf && \
    cp /var/lib/dist/mdb-idm-service.wsgi /opt/yandex/mdb-idm-service && \
    cp /var/lib/dist/uwsgi.deb.ini /opt/yandex/mdb-idm-service/uwsgi.ini && \
    rm -rf /var/lib/dist && \
    rm -rf /root/.cache

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
