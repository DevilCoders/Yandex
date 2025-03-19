FROM dbaas-infra-tests-base
LABEL image-cleanup=true

RUN ln -s /code/fake_juggler.py /opt/yandex/
RUN ln -s /config/fake_juggler.conf /etc/supervisor/conf.d/fake_juggler.conf
RUN ln -s /config/uwsgi.ini /opt/yandex/uwsgi.ini
RUN ln -s /config/config.py /opt/yandex/config.py

ADD src/fake_juggler.py /code/fake_juggler.py
ADD config /config/
