{% set backup_name = salt.mdb_mongodb.backup_name() %}
{% set log_path = [salt.mdb_mongodb.walg_logdir(), 'purge.log' ] | join('/') %}
{% set timeout_mins = salt.mdb_mongodb.walg_purge_timeout_minutes() %}

do-walg-backup-delete:
    cmd.run:
        - name: /usr/local/bin/walg_wrapper.sh -l /etc/wal-g/zk-flock-purge.json -w {{ timeout_mins }} -t {{ timeout_mins }} backup_delete {{ backup_name }} >> {{ log_path }} 2>&1
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8
        - runas: mdb-backup
        - group: mdb-backup

do-walg-oplog-purge:
    cmd.run:
        - name: /usr/local/bin/walg_wrapper.sh -l /etc/wal-g/zk-flock-purge.json -w {{ timeout_mins }} -t {{ timeout_mins }} oplog_purge >> {{ log_path }} 2>&1
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8
        - runas: mdb-backup
        - group: mdb-backup
        - require:
            - do-walg-backup-delete
