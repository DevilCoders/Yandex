start-failover:
{% if salt.pillar.get('ensure_not_master') or salt.pillar.get('data:redis:config:cluster-enabled') == 'yes' %}
    cmd.run:
        - name: /usr/local/yandex/ensure_no_primary.sh
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8
{% else %}
    mdb_redis.start_failover
{% endif %}
