FROM base-deploy
LABEL image-cleanup=true

RUN apt-get update -qq -y --allow-insecure-repositories && \
        apt-get install -y --allow-unauthenticated \
        supervisor

COPY ./config/supervisord.conf /etc/supervisor/supervisord.conf

COPY ./bin/mdb-secrets /usr/local/sbin/mdb-secrets
COPY ./config/mdb-secrets.conf /etc/supervisor/conf.d/mdb-secrets.conf
COPY ./config/mdb-secrets.yaml /etc/mdb-secrets.yaml

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
