{% from slspath ~ "/map.jinja" import walg, s3 with context %}

do-redis-walg-backup:
    cmd.run:
        - name: /usr/local/bin/walg_wrapper.sh -l /etc/wal-g/zk-flock-backup.json -t {{walg.backup_timeout_mins}} backup_create >>{{walg.logdir}}/backup.log 2>&1
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8
        - runas: mdb-backup
        - group: mdb-backup
        - require:
            - test: walg-ready
{# ready for redis #}
