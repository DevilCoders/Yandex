resetup_mongod_host:
    mdb_mongodb.ensure_resetup_host:
        - service: mongod
        - resetup_id: {{ salt.pillar.get('resetup-id')|yaml_encode }}
