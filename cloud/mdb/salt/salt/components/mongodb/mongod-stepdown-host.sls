stepdown_mongod_host:
    mdb_mongodb.ensure_stepdown_host:
        - service: mongod
        - stepdown_id: {{ salt.pillar.get('stepdown-id')|yaml_encode }}
