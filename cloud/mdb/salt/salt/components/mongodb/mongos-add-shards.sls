{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

mongos-shards-add-req:
    test.nop

mongos-shards-added:
    test.nop

{% for shard_name, shard_url in mongodb.add_shards.items() %}
add-shard-{{ shard_name|yaml_encode }}:
    mongodb.shard_present:
      - name: {{ shard_name|yaml_encode }}
      - url: {{ shard_url|yaml_encode }}
      - host: {{ salt.grains.get('fqdn') }}
      - port: {{ mongodb.config.mongos.net.port }}
      - user: admin
      - password: {{ salt.pillar.get('data:mongodb:users:admin:password')|yaml_encode }}
      - authdb: admin
      - require:
        - test: mongos-shards-add-req
      - require_in:
        - test: mongos-shards-added
{% endfor %}


ensure-shard-zones:
    mdb_mongodb.ensure_shard_zones:
      - service: mongos
      - require:
        - test: mongos-shards-added


include:
    - .mongos-sync-check-collections

extend:
    mongos-sync-check-collections-req:
        test.nop:
          - require:
            - mdb_mongodb: ensure-shard-zones
