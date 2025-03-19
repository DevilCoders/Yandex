do-cache-user-object:
    mdb_clickhouse.cache_user_object:
        - object_name: {{ salt['pillar.get']('object_name') }}
        - object_type: {{ salt['pillar.get']('object_type') }}
