{% from "components/mongodb/walg/map.jinja" import walg with context %}

{% set log_file = 'regular_backup.log' %}
{% set is_permanent_flag = '' %}
{% set backup_id = '' %}
{% if salt.mdb_mongodb.backup_service_enabled() %}
{%   set backup_id = salt.mdb_mongodb.backup_id() %}
{%   if salt.mdb_mongodb.backup_is_permanent() %}
{%     set log_file = 'backup.log' %}
{%     set is_permanent_flag = '--permanent' %}
{%   endif %}
{% else %}
{%   if walg.backup_id != 'none' %}
{%     set backup_id = walg.backup_id %}
{%   endif %}
{%   set log_file = 'backup.log' %}
{%   set is_permanent_flag = '--permanent' %}
{% endif %}

{% set log_path = [salt.mdb_mongodb.walg_logdir(), log_file ] | join('/') %}
{% set timeout_mins = salt.mdb_mongodb.walg_backup_timeout_minutes() %}

create-walg-config:
    file.managed:
        - name: /etc/wal-g/wal-g-{{ backup_id }}.yaml
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g.yaml
        - user: root
        - group: s3users
        - mode: 640

delete-walg-config:
    file.absent:
        - name: /etc/wal-g/wal-g-{{ backup_id }}.yaml

do-walg-backup:
    cmd.run:
        - name: /usr/local/bin/walg_wrapper.sh -l /etc/wal-g/zk-flock-backup.json -w {{ timeout_mins }} -t {{ timeout_mins }} backup_create {{ is_permanent_flag }} {{ backup_id }} >> {{ log_path }} 2>&1
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8
        - runas: mdb-backup
        - group: mdb-backup
        - require:
            - create-walg-config
        - require_in:
            - delete-walg-config
