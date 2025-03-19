disable-sentinel-cron:
    file.absent:
        - name: /etc/cron.d/wd-redis-sentinel
