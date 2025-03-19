/etc/cron.d/backup_of_backup:
  file.managed:
    - contents: |
        0 21 * * * root rsync -avh /opt/backup/mysql rsync://space01e.kp.yandex.net:/backup/ >> /var/log/backup_of_backup.log

/etc/logrotate.d/backup_of_backup:
  file.managed:
    - contents: |
        /var/log/backup_of_backup.log {
          rotate 12
          monthly
          compress
          missingok
          notifempty
        }
