FROM dbaas-infra-tests-base
LABEL image-cleanup=true

RUN apt-get update && \
    apt-get -y install dnsmasq && \
    pip3 install docker certifi==2021.10.8

RUN ln -s /code/discovery.py /opt/yandex/
RUN ln -s /config/discovery.conf /etc/supervisor/conf.d/
RUN ln -s /config/dnsmasq.conf /etc/supervisor/conf.d/
RUN rm -f /etc/supervisor/conf.d/nginx.conf
RUN touch /etc/docker-hosts.list
