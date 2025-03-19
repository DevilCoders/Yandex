{% from slspath ~ "/map.jinja" import mongodb, min_oplog_version, walg, s3 with context %}

do-mongo-walg-backup:
    cmd.run:
        - name: /usr/local/bin/walg_wrapper.sh -l /etc/wal-g/zk-flock-backup.json -t {{walg.backup_timeout_mins}} backup_create >>{{walg.logdir}}/backup.log 2>&1
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8
        - runas: mdb-backup
        - group: mdb-backup
        - require:
            - test: walg-ready
{%        for srv in mongodb.services_deployed %}
            - {{srv}}-ready-for-user
{%        endfor %}
