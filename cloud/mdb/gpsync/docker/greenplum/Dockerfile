FROM gpsyncbase:latest
ARG gp_major
ARG gp_version

RUN apt-get update && \
    apt-get install \
        binutils \
        iproute2 \
        tzdata \
        greenplum-db-$gp_major=$gp_version && \
    adduser --system --disabled-password gpadmin && \
    mkdir -p /home/gpadmin/.ssh /var/lib/greenplum/data1/master /var/lib/greenplum/data1/primary /var/lib/greenplum/data1/mirror && \
    cp /root/.ssh/authorized_keys /home/gpadmin/.ssh/authorized_keys && \
    cp /root/.ssh/id_rsa /home/gpadmin/.ssh/id_rsa && \
    echo "StrictHostKeyChecking accept-new" > /home/gpadmin/.ssh/config && \
    echo "*:*:*:gpadmin:123456" > /home/gpadmin/.pgpass && \
    cat /var/lib/dist/docker/greenplum/bashrc >> /home/gpadmin/.bashrc && \
    chown -R gpadmin:gpadmin /home/gpadmin /var/lib/greenplum && chmod -R og-rwx /home/gpadmin /var/lib/greenplum/ && \
    cp /var/lib/dist/docker/greenplum/pam_limits.conf /etc/security/limits.d/gpadmin.conf
