FROM dbaas-infra-tests-base
LABEL image-cleanup=true

RUN ln -s /config/fake_dbm.conf /etc/supervisor/conf.d/
RUN ln -s /config/queue_cleaner.conf /etc/supervisor/conf.d/
RUN ln -s /code/fake_dbm.py /opt/yandex/
RUN ln -s /code/queue_cleaner.py /opt/yandex/
RUN ln -s /code/arbiter.py /opt/yandex/
RUN ln -s /config/uwsgi.ini /opt/yandex/
RUN ln -s /config/config.py /opt/yandex/
