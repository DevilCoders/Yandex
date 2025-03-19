stepdown_mongocfg_host:
    mdb_mongodb.ensure_stepdown_host:
        - service: mongocfg
        - stepdown_id: {{ salt.pillar.get('stepdown-id')|yaml_encode }}
