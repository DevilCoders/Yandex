{% set to_delete = salt.pillar.get('to_delete') %}

delete_cluster_node:
    mdb_redis.delete_cluster_node:
        - host: {{ to_delete }}
