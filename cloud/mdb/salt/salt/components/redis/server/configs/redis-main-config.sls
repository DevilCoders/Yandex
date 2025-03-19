{% from "components/redis/server/map.jinja" import redis with context %}
include:
    - components.redis.common.redispass
    - components.redis.common.info


redis-main-config:
    file.managed:
        - name: /etc/redis/redis-main.conf
        - template: jinja
        - source: salt://components/redis/server/conf/redis-main.conf
        - defaults:
            config: {{ redis.config|json }}
