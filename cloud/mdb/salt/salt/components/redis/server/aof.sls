{% if salt.mdb_redis.is_managed_cluster() %}

{% set aof_enabled = salt.mdb_redis.is_aof_enabled() %}

include:
    - .conf-reload

{%     if aof_enabled %}
/usr/local/yandex/redis_rewrite_aof.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/redis_rewrite_aof.py
        - mode: 755
        - makedirs: True
        - require_in:
            - file: /etc/cron.d/mdb-redis-rewrite-aof

/etc/cron.d/mdb-redis-rewrite-aof:
    file.managed:
        - source: salt://{{ slspath }}/conf/redis_rewrite_aof.cron
        - mode: 755
        - makedirs: True
        - template: jinja
        - require:
            - file: /usr/local/yandex/redis_rewrite_aof.py
            - mdb_redis: redis-conf-reload
{%     else %}
/var/lib/redis/appendonly.aof:
    file.absent:
        - require:
            - mdb_redis: redis-conf-reload
            - file: /etc/cron.d/mdb-redis-rewrite-aof

/var/lib/redis/appendonlydir:
    file.absent:
        - require:
            - mdb_redis: redis-conf-reload
            - file: /etc/cron.d/mdb-redis-rewrite-aof

/etc/cron.d/mdb-redis-rewrite-aof:
    file.absent
{%     endif %}

{% endif %}
