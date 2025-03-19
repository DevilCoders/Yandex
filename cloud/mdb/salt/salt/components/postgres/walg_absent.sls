wal-g:
    pkg.purged

/etc/wal-g:
    file.absent

/etc/wal-g-backup-push.conf:
    file.absent

/usr/local/yandex/pg_walg_backup_push.py:
    file.absent

/etc/cron.d/pg_walg_backup_push:
    file.absent

