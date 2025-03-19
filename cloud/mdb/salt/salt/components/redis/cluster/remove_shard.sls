{% set deleted_hosts = salt.pillar.get('deleted_hosts') %}

remove_shard:
    mdb_redis.remove_shard:
        - shard_hosts: {{ deleted_hosts | tojson }}
