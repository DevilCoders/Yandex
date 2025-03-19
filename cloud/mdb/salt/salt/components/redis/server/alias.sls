{% from slspath ~ "/map.jinja" import redis, tls_folder with context %}
{% set redis_cmd = "redis-cli --server-config /etc/redis/redis-main.conf" %}

redis-root-bashrc:
    file.accumulated:
        - name: root-bashrc
        - filename: /root/.bashrc
        - text: |-
            # Some useful things for redis
            export REDISCLI_AUTH=$(python3 -c 'import json; print(json.load(open("{{ salt.mdb_redis.get_redispass_file() }}"))["password"])')
            alias {{ redis.cli }}='{{ redis_cmd }} -p {{ redis.config.port }}'
{% if redis.tls.enabled %}
            alias {{ redis.tls.cli }}='{{ redis_cmd }} -p {{ redis.tls.port }} --tls --cacert {{ redis.config['tls-ca-cert-file'] }}'
{% else %}
            unalias {{ redis.tls.cli }} > /dev/null 2>&1 || /bin/true
{% endif %}
            alias wal-g-redis='wal-g-redis --config /etc/wal-g/wal-g.yaml'
        - require_in:
            - file: /root/.bashrc
