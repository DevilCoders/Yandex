FROM dbaas-infra-tests-base
LABEL image-cleanup=true

RUN apt-get install -y python3-cryptography python3-requests python3-six python3-sqlalchemy socat && \
    pip install pykmip && mkdir -p /etc/pykmip/ && pip install --upgrade cryptography==3.2.1 && \
    rm -f /etc/supervisor/conf.d/nginx.conf
# apt-get install -y python3-dev libffi-dev libssl-dev libsqlite3-dev

COPY ./config/ /etc/pykmip/
COPY ./config/pykmip.supervisor.conf /etc/supervisor/conf.d/pykmip.conf
COPY ./config/socat.supervisor.conf /etc/supervisor/conf.d/socat.conf

EXPOSE 5696

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
