FROM dbaas-infra-tests-base
LABEL image-cleanup=true

RUN pip3 install aiosmtpd aiohttp && \
    rm -f /etc/supervisor/conf.d/nginx.conf && \
    ln -s /config/fake_smtp.conf /etc/supervisor/conf.d/ && \
    ln -s /code/fake_smtp.py /opt/yandex/

EXPOSE 5000 6000
