do-walg-backup-delete:
    cmd.run:
        - name: > 
            /usr/bin/wal-g-mysql backup-list --config=/etc/wal-g/wal-g.yaml | grep {{ salt['pillar.get']('backup_name') }}
            && /usr/local/yandex/mysql_walg.py delete --target="{{ salt['pillar.get']('backup_name') }}" >> /var/log/mysql/s3-backup.log 2>&1
            || true
        - runas: mysql
        - group: mysql
