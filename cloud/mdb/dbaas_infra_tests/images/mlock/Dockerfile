FROM base-deploy
LABEL image-cleanup=true

RUN apt-get update -qq -y --allow-insecure-repositories && \
        apt-get install -y --allow-unauthenticated \
        supervisor

COPY ./config/supervisord.conf /etc/supervisor/supervisord.conf

COPY ./bin/mlock /usr/local/sbin/mlock
COPY ./config/mlock.conf /etc/supervisor/conf.d/mlock.conf
COPY ./config/mlock.yaml /etc/mlock.yaml

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
