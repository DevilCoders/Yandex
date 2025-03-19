FROM dbaas-infra-tests-base
LABEL image-cleanup=true

RUN ln -s /code/fake_solomon.py /opt/yandex/
RUN ln -s /config/uwsgi.ini /opt/yandex/
RUN ln -s /config/config.py /opt/yandex/
RUN ln -s /config/fake_solomon.conf /etc/supervisor/conf.d/fake_solomon.conf
