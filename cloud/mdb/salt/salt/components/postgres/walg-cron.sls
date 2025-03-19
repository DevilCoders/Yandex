/etc/cron.d/pg_walg_backup_push:
{% if not salt.pillar.get('data:backup:use_backup_service') %}
    file.managed:
        - source: salt://{{ slspath }}/conf/cron.d/pg_walg_backup_push
        - template: jinja
        - mode: 644
{% else %}
    file.absent
{% endif %}
