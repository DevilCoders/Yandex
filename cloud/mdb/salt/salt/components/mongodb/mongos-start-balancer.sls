start_mongos_balancer:
    mdb_mongodb.ensure_balancer_state:
        - service: mongos
        - started: true
