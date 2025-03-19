FROM registry.yandex.net/ubuntu:bionic

WORKDIR /

COPY ydb /usr/bin/
COPY logbroker /usr/bin/
COPY files/client_profile/* /root/.ydb/config/
COPY files/run_scripts.sh /usr/local/bin/

CMD sleep inf
