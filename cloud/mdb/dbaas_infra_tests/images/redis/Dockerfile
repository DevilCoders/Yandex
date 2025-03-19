FROM redis-bionic-template:latest
LABEL image-cleanup=true
ENV SALT_VERSION=3002.7+ds-1+yandex0
ENV MSGPACK_VERSION=3-4fc9b70

RUN for i in $(seq 10); do apt-get update -qq && break || sleep 1; done
RUN apt-get install -y python-msgpack=${MSGPACK_VERSION} python-concurrent.futures python-boto python-botocore python3-boto
RUN apt-get install -y salt-common=${SALT_VERSION} salt-minion=${SALT_VERSION}
RUN rm -rf /etc/salt/minion_id /etc/salt/pki/minion/minion.p*

ADD salt-minion.conf /etc/init/salt-minion.conf
ADD config/named.conf.options /etc/bind/named.conf.options

RUN rm -rf /lib/systemd/system/acpid* \
           /lib/systemd/system/blk* \
           /lib/systemd/system/dm* \
           /lib/systemd/system/console-setup* \
           /lib/systemd/system/ifup* \
           /lib/systemd/system/ipmi* \
           /lib/systemd/system/keyboard* \
           /lib/systemd/system/lm-sensors* \
           /lib/systemd/system/lvm* \
           /lib/systemd/system/mdadm* \
           /lib/systemd/system/mdmon* \
           /lib/systemd/system/netplan* \
           /lib/systemd/system/networking* \
           /lib/systemd/system/setvtrgb* \
           /lib/systemd/system/smart* \
           /lib/systemd/system/systemd-udev* \
           /lib/systemd/system/systemd-update-utmp* \
           /lib/systemd/system/systemd-hostnamed* \
           /lib/systemd/system/mdb-metrics* \
           /lib/systemd/system/udev*

RUN ln -s /dev/null /etc/systemd/system/system-getty.slice

RUN echo 'precedence ::ffff:0:0/96  100' > /etc/gai.conf

RUN touch /root/.ssh/authorized_keys

COPY config/allCAs.pem /opt/yandex/allCAs.pem
COPY config/allCAs.pem /opt/yandex/overrideCAs.pem

ENV container docker
CMD ["/lib/systemd/systemd"]
