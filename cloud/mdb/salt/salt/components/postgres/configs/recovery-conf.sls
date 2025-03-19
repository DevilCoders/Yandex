{% from "components/postgres/pg.jinja" import pg with context %}

recovery.conf:
    postgresql_cmd.populate_recovery_conf:
{% if pg.version.major_num < 1200 %}
        - name: {{ pg.data }}/recovery.conf
{%  else %}
        - name: {{ pg.data }}/conf.d/recovery.conf
{% endif %}
        - use_replication_slots: {{ salt['pillar.get']('data:use_replication_slots', True) }}
        - application_name: '{{ salt['grains.get']('id')|replace('.', '_')|replace('-', '_') }}'
        - recovery_min_apply_delay: {{ salt['pillar.get']('data:config:recovery_min_apply_delay', '') }}
        - use_restore_command: {{ (pg.version.major_num < 1200) | to_bool }}
