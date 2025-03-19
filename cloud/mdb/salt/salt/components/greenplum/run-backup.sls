{% from "components/greenplum/map.jinja" import gpdbvars with context %}
{% if not salt.pillar.get('data:backup:use_backup_service') %}
do_s3_backup:
    cmd.run:
        - name: >
            flock -o /tmp/gp_walg_backup_push.lock
            /usr/local/yandex/gp_walg_backup_push.py
{% if salt['pillar.get']('delta_from_previous_backup', True) %}
            --delta-from-previous-backup
{% endif %}
{% if salt['pillar.get']('user_backup') %}
            --user-backup
{% endif %}
{% if salt['pillar.get']('backup_id') %}
            --id {{ salt['pillar.get']('backup_id') }}
{% endif %}
{% if salt['pillar.get']('from_backup_id') %}
            --delta-from-id {{ salt['pillar.get']('from_backup_id') }}
{% endif %}
{% if salt['pillar.get']('full_backup') %}
            --full
{% endif %}
{% if salt['pillar.get']('no_sleep', False) %}
            --no-sleep
{% endif %}
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
{% else %}
do_s3_backup:
    cmd.run:
        - name: >
            flock -o /tmp/gp_walg_backup_push.lock
            /usr/local/yandex/gp_walg_backup_push.py
{% if salt['pillar.get']('user_backup') %}
            --user-backup
{% endif %}
{% if salt['pillar.get']('backup_id') %}
            --id {{ salt['pillar.get']('backup_id') }}
{% endif %}
{% if salt['pillar.get']('from_backup_name') %}
            --delta-from-name {{ salt['pillar.get']('from_backup_name') }}
{% else %}
{% if salt['pillar.get']('from_backup_id') %}
            --delta-from-id {{ salt['pillar.get']('from_backup_id') }}
{% endif %}
{% endif %}
{% if salt['pillar.get']('full_backup') %}
            --full
{% endif %}
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
{% endif %}
