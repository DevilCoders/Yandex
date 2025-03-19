FROM base-deploy
LABEL image-cleanup=true

RUN apt-get update -qq -y --allow-insecure-repositories && \
        apt-get install -y \
        nginx-common \
        nginx-full \
        supervisor \
        yandex-passport-tvmtool

COPY ./config/allCAs.pem /opt/yandex/mdb-infra-test-allCAs.pem

# supervisor
COPY ./config/supervisord.conf /etc/supervisor/supervisord.conf

# nginx
COPY ./config/nginx.supervisor.conf /etc/supervisor/conf.d/nginx.conf
RUN mkdir -p /etc/nginx/ssl
COPY ./config/dhparam.pem /etc/nginx/ssl/dhparam.pem
COPY ./config/nginx.conf /etc/nginx/nginx.conf

# mdb-deploy-api
COPY ./bin/mdb-deploy-api /mdb-deploy-api
COPY ./config/mdb-deploy-api.supervisor.conf /etc/supervisor/conf.d/mdb-deploy-api.conf
COPY ./config/mdb-deploy-api.yaml /mdb-deploy-api.yaml
COPY ./config/dbpg.yaml /dbpg.yaml
COPY ./config/mdb-deploy-api.nginx.conf /etc/nginx/conf.d/mdb-deploy-api.conf
COPY ./config/mdb-deploy-api.pem /etc/nginx/ssl/mdb-deploy-api.pem
COPY ./config/mdb-deploy-api.key /etc/nginx/ssl/mdb-deploy-api.key

# TVM tool
COPY ./config/tvmtool.conf /etc/tvmtool/tvmtool.conf
COPY ./config/tvmtool.supervisor.conf /etc/supervisor/conf.d/tvmtool.conf

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
