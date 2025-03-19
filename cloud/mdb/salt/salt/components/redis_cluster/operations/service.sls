{% set is_walg_enabled = salt.mdb_redis.is_walg_enabled() %}

include:
{% if salt.pillar.get('service-restart') %}
    - components.redis.server.restart
{% endif %}
{% if is_walg_enabled %}
    - components.redis.walg.config
    - components.redis.walg.redispass
{% endif %}
    - components.redis.server.service-reload
{% if salt.pillar.get('data:mdb_metrics:enabled', True) %}
    - components.redis.server.mdb-metrics
    - components.mdb-metrics.service
{% endif %}
{% if salt.pillar.get('data:redis:config:cluster-enabled') != 'yes' %}
    - components.redis.sentinel.configs.sentinel-config
    - components.redis.sentinel.service
{% endif %}
