FROM dbaas-infra-tests-base
LABEL image-cleanup=true
RUN pip3 install pyOpenSSL==17.3.0 cryptography==3.2.1

RUN ln -s /config/fake_certificator.conf /etc/supervisor/conf.d/
RUN ln -s /code/fake_certificator.py /opt/yandex/
RUN ln -s /code/ca.py /opt/yandex/
RUN ln -s /config/uwsgi.ini /opt/yandex/
RUN ln -s /config/config.py /opt/yandex/
