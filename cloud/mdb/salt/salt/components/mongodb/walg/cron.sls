/etc/cron.d/wal-g:
{% if not salt.pillar.get('data:backup:use_backup_service') %}
    file.managed:
        - source: salt://{{ slspath }}/conf/wal-g.cron
        - template: jinja
        - mode: 644
{% else %}
    file.absent
{% endif %}
