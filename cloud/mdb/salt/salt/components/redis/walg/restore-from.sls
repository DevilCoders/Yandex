{% from slspath ~ "/map.jinja" import walg with context %}
{% from "components/redis/server/map.jinja" import redis with context %}


walg-restore-dir:
    file.directory:
        - name: {{walg.restore_confdir}}
        - user: root
        - group: s3users
        - mode: 0750
        - makedirs: True
        - require:
            - group: mdb-s3users-group

walg-pgp-key:
    file.managed:
        - name: {{walg.restore_confdir}}/PGP_KEY
        - user: root
        - group: s3users
        - mode: 640
        - contents: |
            {{salt.pillar.get('data:restore-from-pillar-data:s3:gpg_key') | indent(12)}}
        - require:
            - pkg: walg-packages
            - file: walg-restore-dir

walg-restore-config:
    file.managed:
        - name: {{walg.restore_confdir}}/wal-g.yaml
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g-restore.yaml
        - user: root
        - group: s3users
        - mode: 640
        - require:
            - file: walg-restore-dir
            - pkg: walg-packages


walg-restore-ready:
  test.nop:
    - require:
        - pkg: walg-packages
        - file: walg-restore-config
        - file: /usr/local/bin/walg_wrapper.sh
        - file: walg-logdir
        - file: walg-pgp-key

{% set restore_backup = salt.pillar.get('restore-from:backup-name', salt.pillar.get('restore-from:backup-id')) %}
do-walg-restore:
    cmd.run:
        - name: >
            /usr/local/bin/walg_wrapper.sh -c {{walg.restore_confdir}} -l {{walg.confdir}}/zk-flock-restore.json -t {{walg.restore_timeout_mins}} backup_restore {{restore_backup}} >>{{walg.logdir}}/restore.log 2>&1 && touch /tmp/restore-from-walg-success
        - require:
            - test: walg-ready
            - test: walg-restore-ready
{% if salt.pillar.get('do-backup') and salt.mdb_redis.walg_can_backup() %}
            - cmd: do-redis-walg-backup
{% endif %}
        - unless:
            - ls /tmp/restore-from-walg-success

fix-dump-rights:
    file.directory:
        - name: {{ salt.mdb_redis.get_redis_data_folder() }}
        - user: {{ redis.user }}
        - group: {{ redis.group }}
        - mode: 750
        - file_mode: 644
        - recurse:
            - user
            - group
            - mode
        - require:
            - cmd: do-walg-restore

purge-walg-restore-confdir:
    cmd.run:
        - name: rm -rf {{walg.restore_confdir}}
        - require:
            - file: fix-dump-rights
