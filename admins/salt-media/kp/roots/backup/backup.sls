/opt/backup/backup_hosts/conf.d:
  file.recurse:
    - source: salt://{{ slspath }}/files/backup-conf.d
    - make_dirs: true

/opt/backup/backup_hosts/backup.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/backup.sh
    - mode: 755

/etc/cron.d/backup_data:
  file.managed:
    - contents: |
        # master, CADMIN-6900
        30 4 * * * root flock -w 1 /var/tmp/backup-kp-master01e.kp.yandex.net.lock /opt/backup/backup_hosts/backup.sh bo.kinopoisk.ru

/etc/logrotate.d/backup_data:
  file.managed:
    - contents: |
        /opt/backup/backup_hosts/logs/*.log {
          rotate 14
          daily
          compress
          missingok
          notifempty
        }
