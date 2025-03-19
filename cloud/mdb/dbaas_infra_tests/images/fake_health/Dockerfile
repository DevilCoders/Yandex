FROM dbaas-infra-tests-base
LABEL image-cleanup=true

RUN ln -s /code/fake_health.py /opt/yandex/ && \
    ln -s /config/fake_health.conf /etc/supervisor/conf.d/fake_health.conf && \
    ln -s /config/uwsgi.ini /opt/yandex/uwsgi.ini && \
    ln -s /config/config.py /opt/yandex/config.py

RUN mkdir -m 750 -p /etc/nginx/ssl && chown root:www-data /etc/nginx/ssl
RUN ln -s /config/cert.pem /etc/nginx/ssl/cert.pem
RUN ln -s /config/key.pem /etc/nginx/ssl/key.pem

ADD src/fake_health.py /code/fake_health.py
ADD config /config/
