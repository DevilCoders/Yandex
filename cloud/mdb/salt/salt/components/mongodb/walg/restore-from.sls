{% from slspath ~ "/map.jinja" import mongodb, min_oplog_version, walg with context %}
{% set mongodb_version = salt.mdb_mongodb.version() %}

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


{% set host = salt.grains.get('id') %}
{% set admin_user = 'admin' %}
{% set admin_password = mongodb.users[admin_user].password %}
{% for srv in mongodb.services_deployed if srv != 'mongos' %}
{%   set port = mongodb.config.get(srv).net.port %}
mongodb-set-low-wt-cache-{{srv}}:
    module.run:
        - name: mongodb.set_wt_engine_config
        - cfg: {{ salt.mdb_mongodb.wt_engine_config_from_cache_bytes(salt.mdb_mongodb.wt_cache_bytes_for_restore()) }}
        - user: {{admin_user}}
        - password: {{admin_password}}
        - database: 'admin'
        - authdb: 'admin'
        - host: {{host}}
        - port: {{port}}
        - require:
            - {{srv}}-ready-for-user

mongodb-set-original-wt-cache-{{srv}}:
    module.run:
        - name: mongodb.set_wt_engine_config
        - cfg: {{ salt.mdb_mongodb.wt_engine_config_from_cache_bytes(salt.mdb_mongodb.wt_cache_bytes()) }}
        - user: {{admin_user}}
        - password: {{admin_password}}
        - database: 'admin'
        - authdb: 'admin'
        - host: {{host}}
        - port: {{port}}
        - require:
            - {{srv}}-ready-for-user
{% endfor %}

walg-restore-ready:
  test.nop:
    - require:
        - pkg: walg-packages
        - file: walg-restore-config
        - file: /usr/local/bin/walg_wrapper.sh
        - file: walg-logdir
        - file: walg-pgp-key
{% for srv in mongodb.services_deployed if srv != 'mongos' %}
        - module: mongodb-set-low-wt-cache-{{srv}}
{% endfor %}

{% set restore_backup = salt.pillar.get('restore-from:backup-name', salt.pillar.get('restore-from:backup-id')) %}
do-walg-restore:
    cmd.run:
        - name: >
            /usr/local/bin/walg_wrapper.sh -c {{walg.restore_confdir}} -l {{walg.confdir}}/zk-flock-restore.json -t {{walg.restore_timeout_mins}} backup_restore {{restore_backup}} >>{{walg.logdir}}/restore.log 2>&1 && touch /tmp/restore-from-walg-success
        - require:
            - test: walg-ready
            - test: walg-restore-ready
{% if salt.pillar.get('do-backup') and not salt.pillar.get('data:backup:use_backup_service') %}
            - cmd: do-mongo-walg-backup
{% endif %}
        - require_in:
{% for srv in mongodb.services_deployed if srv != 'mongos' %}
            - module: mongodb-set-original-wt-cache-{{srv}}
{% endfor %}
        - unless:
            - ls /tmp/restore-from-walg-success

{% if (mongodb_version.major_num | int) >= min_oplog_version and walg.oplog_replay %}
{%  set pitr = salt.pillar.get('restore-from:pitr') %}
{%  set replay_from = '{ts}.{inc}'.format(ts=pitr.backup_begin.timestamp, inc=pitr.backup_begin.inc) %}
{%  set replay_until = '{ts}.{inc}'.format(ts=pitr.target.timestamp, inc=pitr.target.inc) %}

do-walg-replay:
    cmd.run:
        - name: >
            /usr/local/bin/walg_wrapper.sh -c {{walg.restore_confdir}} -l {{walg.confdir}}/zk-flock-restore.json -t {{walg.restore_timeout_mins}} oplog_replay {{replay_from}} {{replay_until}} >>{{walg.logdir}}/restore.log 2>&1 && touch /tmp/replay-from-walg-success
        - require:
            - cmd: do-walg-restore
        - require_in:
            - cmd: purge-walg-restore-confdir
        - unless:
            - ls /tmp/replay-from-walg-success
{%  endif %}

purge-walg-restore-confdir:
    cmd.run:
        - name: rm -rf {{walg.restore_confdir}}
        - require:
            - cmd: do-walg-restore

walg-restore-completed:
    test.nop:
        - require:
            - cmd: purge-walg-restore-confdir
        - require-in:
            - file: wal-g-oplog-push-service
