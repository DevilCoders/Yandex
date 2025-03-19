ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

ARG salt_version=3002.9+ds-1
ENV DEBIAN_FRONTEND=noninteractive

COPY config/salt-archive-keyring.gpg /usr/share/keyrings/salt-archive-keyring.gpg

RUN apt update && apt install ca-certificates -y

RUN echo "deb [signed-by=/usr/share/keyrings/salt-archive-keyring.gpg] https://repo.saltproject.io/py3/ubuntu/18.04/amd64/3002 bionic main" >> /etc/apt/sources.list.d/salt.list
RUN apt-get update -qq -y --allow-insecure-repositories && \
    apt-get upgrade -y --allow-unauthenticated && \
    apt-get install -y --allow-unauthenticated \
        python3-requests \
        python3-cherrypy3 \
        python3-openssl \
        python3-gnupg \
        python3-nacl \
        python3-paramiko \
        python3-redis \
        python3-git \
        python3-jwt \
        python3-lxml \
        salt-master=$salt_version \
        salt-api=$salt_version \
        salt-common=$salt_version \
        supervisor \
        s3cmd \
        cron \
        python3-pip
RUN pip3 install superlance

# supervisor
COPY config/supervisord.conf /etc/supervisor/supervisord.conf

# salt-master
COPY config/master.supervisor.conf /etc/supervisor/conf.d/salt-master.conf

# salt-api
COPY config/salt-api.supervisor.conf /etc/supervisor/conf.d/salt-api.conf

# update-salt-image
COPY build/update_salt_image.py /etc/cron.yandex/update_salt_image.py
RUN chmod 755 /etc/cron.yandex/update_salt_image.py
RUN chmod 755 /etc/cron.yandex
COPY config/update_salt_image.cron /etc/cron.d/update_salt_image
RUN chmod 600 /etc/cron.d/update_salt_image
RUN crontab /etc/cron.d/update_salt_image

COPY salt-master/modules /etc/salt/master_modules

COPY config/cron.supervisor.conf /etc/supervisor/conf.d/cron.conf

# mdb-deploy-saltkeys
COPY build/mdb-deploy-saltkeys /opt/yandex/mdb-deploy-saltkeys/mdb-deploy-saltkeys
COPY config/mdb-deploy-saltkeys.supervisor.conf /etc/supervisor/conf.d/mdb-deploy-saltkeys.conf

COPY config/entrypoint.sh /entrypoint.sh

# salt data
RUN rm -rf /srv
RUN mkdir /srv-images
RUN ln -s /srv-images srv

CMD ["/entrypoint.sh"]
