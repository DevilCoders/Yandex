from registry.yandex.net/ubuntu:trusty

ADD 01-base_layer.sh /
ADD 02-app_bin_layer.sh /
ADD 03-conf_layer.sh /

RUN time /bin/bash /01-base_layer.sh
RUN time /bin/bash /02-app_bin_layer.sh
RUN time /bin/bash /03-conf_layer.sh

CMD /usr/sbin/runsvdir-start
