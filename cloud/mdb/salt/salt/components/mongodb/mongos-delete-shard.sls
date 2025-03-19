{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}
{% set host = salt.grains.get('fqdn') %}
{% set admin_password = salt.pillar.get('data:mongodb:users:admin:password') %}

mongos-shard-delete-req:
    test.nop


{% set delete_shard_name = mongodb.get('delete_shard') %}
{% if delete_shard_name %}
mongos-delete-shard:
    mongodb.shard_absent:
      - require:
          - test: mongos-shard-delete-req
      - require_in:
          - mdb_mongodb: mongos-set-balancer-active-window
      - name: {{ delete_shard_name|yaml_encode }}
      - host: {{ host }}
      - port: {{ mongodb.config.mongos.net.port }}
      - user: admin
      - password: {{ admin_password|yaml_encode }}
      - authdb: admin

mongos-unset-balancer-active-window:
    mdb_mongodb.ensure_balancer_active_window_disabled:
      - service: mongos
      - require:
          - mongodb: mongos-deleted-shard-is-not-primary

mongos-balancer-enabled:
    mongodb.check_balancer_enabled:
      - require:
          - mdb_mongodb: mongos-unset-balancer-active-window
      - host: {{ host }}
      - port: {{ mongodb.config.mongos.net.port }}
      - user: admin
      - password: {{ admin_password|yaml_encode }}
      - authdb: admin

mongos-deleted-shard-is-not-primary:
    mongodb.shard_is_not_primary:
      - name: {{ delete_shard_name|yaml_encode }}
      - host: {{ host }}
      - port: {{ mongodb.config.mongos.net.port }}
      - user: admin
      - password: {{ admin_password|yaml_encode }}
      - authdb: admin

include:
    - .mongos-set-balancer-active-window
    - .mongos-start-balancer
    - .mongos-sync-check-collections

extend:
    mongos-shard-delete-req:
        test.nop:
          - require:
              - mongodb: mongos-deleted-shard-is-not-primary
              - mdb_mongodb: mongos-unset-balancer-active-window
              - mdb_mongodb: start_mongos_balancer
              - test: mongos-sync-check-collections-done
{% endif %}
