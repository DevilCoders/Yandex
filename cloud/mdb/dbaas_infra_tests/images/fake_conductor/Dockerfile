FROM dbaas-infra-tests-base
LABEL image-cleanup=true

RUN ln -s /code/fake_conductor.py /opt/yandex/
RUN ln -s /config/uwsgi.ini /opt/yandex/
RUN ln -s /config/config.py /opt/yandex/
RUN ln -s /config/fake_conductor.conf /etc/supervisor/conf.d/fake_conductor.conf
