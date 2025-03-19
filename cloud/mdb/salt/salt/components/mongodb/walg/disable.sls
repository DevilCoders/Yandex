disable-walg-cron:
    file.absent:
        - name: /etc/cron.d/wal-g

