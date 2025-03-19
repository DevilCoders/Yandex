mongocfg-service:
    service.running:
        - name: mongocfg
        - enable: True
        - reload: True

    mdb_mongodb.alive:
        - tries: 10
        - timeout: 5
        - service: mongocfg
        - use_auth: False
        - watch:
            - service: mongocfg-service
