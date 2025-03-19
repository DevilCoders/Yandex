FROM base-deploy
LABEL image-cleanup=true

ENV SALT_VERSION='3002.7+ds-1+yandex0'

RUN apt-get update -qq -y --allow-insecure-repositories && \
        apt-get install -y \
        python-requests \
        nginx-common \
        nginx-full \
        python-cherrypy3 \
        python3-cherrypy3 \
        python-nacl \
        python3-nacl \
        python-jwt \
        python3-jwt \
        python3-lxml \
        python3-paramiko \
        salt-master=${SALT_VERSION} \
        salt-minion=${SALT_VERSION} \
        salt-api=${SALT_VERSION} \
        salt-common=${SALT_VERSION} \
        supervisor

RUN useradd saltapi
RUN echo saltapi:testpwd | chpasswd

COPY ./config/allCAs.pem /opt/yandex/mdb-infra-test-allCAs.pem
COPY ./config/allCAs.pem /opt/yandex/allCAs.pem

# supervisor
COPY ./config/supervisord.conf /etc/supervisor/supervisord.conf

# nginx
COPY ./config/nginx.supervisor.conf /etc/supervisor/conf.d/nginx.conf
RUN mkdir -p /etc/nginx/ssl
COPY ./config/dhparam.pem /etc/nginx/ssl/dhparam.pem
COPY ./config/nginx.conf /etc/nginx/nginx.conf

# salt-master
COPY ./config/master.supervisor.conf /etc/supervisor/conf.d/salt-master.conf
COPY ./config/master /etc/salt/master

# salt-master key pair
COPY ./config/master.pem /etc/salt/pki/master/master.pem
COPY ./config/master.pub /etc/salt/pki/master/master.pub

# salt-master message signing key pair
COPY ./config/master_sign.pem /etc/salt/pki/master/master_sign.pem
COPY ./config/master_sign.pub /etc/salt/pki/master/master_sign.pub

# salt-master message signing public key
COPY ./config/master_sign.pub /etc/salt/pki/minion/master_sign.pub

# salt-api
COPY ./config/salt-api.supervisor.conf /etc/supervisor/conf.d/salt-api.conf
COPY ./config/salt-api.nginx.conf /etc/nginx/conf.d/salt-api.conf
COPY ./config/salt-api.pem /etc/nginx/ssl/salt-api.pem
COPY ./config/salt-api.key /etc/nginx/ssl/salt-api.key

# mdb-deploy-saltkeys
COPY ./bin/mdb-deploy-saltkeys /mdb-deploy-saltkeys
COPY ./config/mdb-deploy-saltkeys.supervisor.conf /etc/supervisor/conf.d/mdb-deploy-saltkeys.conf
COPY ./config/mdb-deploy-saltkeys.yaml /mdb-deploy-saltkeys.yaml

RUN rm -rf /srv && ln -s /code/srv /srv && mkdir -p /etc/salt/salt-master/modules/pillar && \
    ln -s /srv/salt/components/deploy/salt-master/modules/pillar/dbaas_pillar.py /etc/salt/salt-master/modules/pillar/dbaas_pillar.py && \
    ln -s /srv/salt/components/deploy/salt-master/modules/pillar/pgcerts.py /etc/salt/salt-master/modules/pillar/pgcerts.py

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
