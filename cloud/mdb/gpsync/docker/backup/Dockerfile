FROM gpsyncbase:latest
RUN mkdir -p /archive/segment-1 && chown backup.backup /archive && \
    cp /var/lib/dist/docker/backup/rsync.secrets /etc && \
    chmod 0600 /etc/rsync.secrets && \
    cp /var/lib/dist/docker/backup/rsyncd.conf /etc && \
    cp /var/lib/dist/docker/backup/rsync.conf /etc/supervisor/conf.d
