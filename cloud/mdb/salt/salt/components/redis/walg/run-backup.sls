{% from "components/redis/walg/map.jinja" import walg with context %}

create-walg-config:
    file.managed:
        - name: /etc/wal-g/wal-g-{{ walg.backup_id }}.yaml
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g.yaml
        - user: root
        - group: s3users
        - mode: 640

delete-walg-config:
    file.absent:
        - name: /etc/wal-g/wal-g-{{ walg.backup_id }}.yaml

do-walg-backup:
    cmd.run:
        - name: /usr/local/bin/walg_wrapper.sh -l /etc/wal-g/zk-flock-backup.json -w {{walg.backup_timeout_mins}} -t {{walg.backup_timeout_mins}} backup_create --permanent {{ walg.backup_id if walg.backup_id != 'none' else '' }} >> {{walg.logdir}}/backup.log 2>&1
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8
        - runas: mdb-backup
        - group: mdb-backup
        - require:
            - create-walg-config
        - require_in:
            - delete-walg-config
