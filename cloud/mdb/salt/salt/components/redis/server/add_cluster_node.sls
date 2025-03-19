{% set host = salt.grains.get('fqdn') %}

add_cluster_node:
    mdb_redis.add_cluster_node:
        - host: {{ host }}
        - is_master: {{ salt.pillar.get('new_master', False) }}
        - require:
            - service: redis-is-running


{{ salt.mdb_metrics.get_redis_no_slots_marker_filename() }}:
    file.managed:
        - mode: 644
