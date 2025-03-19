{% from "components/redis/walg/map.jinja" import walg with context %}


do-walg-backup-delete:
    cmd.run:
        - name: /usr/local/bin/walg_wrapper.sh -l /etc/wal-g/zk-flock-backup.json -w {{walg.purge_timeout_mins}} -t {{walg.purge_timeout_mins}} backup_delete {{ walg.backup_name }} >> {{walg.logdir}}/purge.log 2>&1
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8
        - runas: mdb-backup
        - group: mdb-backup

do-walg-oplog-purge:
    cmd.run:
        - name: /usr/local/bin/walg_wrapper.sh -l /etc/wal-g/zk-flock-backup.json -w {{walg.purge_timeout_mins}} -t {{walg.purge_timeout_mins}} oplog_purge >> {{walg.logdir}}/purge.log 2>&1
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8
        - runas: mdb-backup
        - group: mdb-backup
        - require:
            - do-walg-backup-delete
