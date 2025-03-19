/etc/cron.d/gp_walg_backup_push:
{% if not salt.pillar.get('data:backup:use_backup_service') %}
    file.managed:
        - source: salt://{{ slspath }}/conf/cron.d/gp_walg_backup_push
        - template: jinja
        - mode: 644
{% else %}
    file.absent
{% endif %}
/etc/cron.d/gp_walg_create_restore_point:
{% if not salt.pillar.get('data:backup:use_backup_service') %}
    file.managed:
        - source: salt://{{ slspath }}/conf/cron.d/gp_walg_create_restore_point
        - template: jinja
        - mode: 644
{% else %}
    file.absent
{% endif %}
