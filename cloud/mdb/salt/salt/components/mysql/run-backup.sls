do_s3_backup:
    cmd.run:
        - name: >
            su mysql -c "
            flock -o /tmp/mysql_walg_backup_push.lock
            /usr/local/yandex/mysql_walg.py backup_push
{% if salt.pillar.get('data:backup:use_backup_service') %}
{% if salt['pillar.get']('backup_id') %}
            --backup_id {{ salt['pillar.get']('backup_id') }}
{% endif %}
{% endif %}
{% if salt['pillar.get']('user_backup') %}
            --user-backup
{% endif %}
            --skip-election-in-zk
            --skip-sleep
            >> /var/log/mysql/s3-backup.log 2>&1
            "
        - cwd: /home/mysql
