resetup_mongocfg_host:
    mdb_mongodb.ensure_resetup_host:
        - service: mongocfg
        - resetup_id: {{ salt.pillar.get('resetup-id')|yaml_encode }}
