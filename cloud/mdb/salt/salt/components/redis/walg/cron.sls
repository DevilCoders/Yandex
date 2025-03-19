/etc/cron.d/wal-g:
{% if salt.mdb_redis.only_walg_enabled() %}
    file.managed:
        - source: salt://{{ slspath }}/conf/wal-g.cron
        - template: jinja
        - mode: 644
{% else %}
    file.absent
{% endif %}
