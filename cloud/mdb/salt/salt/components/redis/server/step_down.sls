{% set host = salt.pillar.get('ensure_not_master') %}

ensure-not-master:
    mdb_redis.ensure_not_master:
        - host: {{ host }}
