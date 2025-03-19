FROM greenplum:latest

RUN apt-get update && \
    apt-get install \
        odyssey=1.2.1rc-2032-481316f-yandex220 && \
    cd /var/lib/dist && \
    cp ./ya_build/gpsync /usr/local/bin/ && cp ./ya_build/gpsync-util /usr/local/bin/ && \
    make -C static install && \
    mkdir -p /etc/gpsync/plugins /etc/odyssey && \
    cp -R docker/gpsync/gpconfigs /home/gpadmin/ && chown -R gpadmin:gpadmin /home/gpadmin/ && \
    cp docker/gpsync/initcluster.sh /home/gpadmin/ && \
    cp docker/gpsync/supervisor.conf /etc/supervisor/conf.d/gpsync.conf && \
    cp docker/gpsync/gpsync.conf /etc/gpsync.conf && \
    cp docker/gpsync/gen_rec_conf.sh /usr/local/bin/gen_rec_conf.sh && \
    cp docker/gpsync/odyssey.conf /etc/odyssey/odyssey.conf && \
    cp docker/gpsync/sudoers /etc/sudoers.d/gpsync
