mongos-service:
    service.running:
        - name: mongos
        - enable: True
        - reload: True

    mdb_mongodb.alive:
        - tries: 10
        - timeout: 5
        - service: mongos
        - use_auth: False
        - watch:
            - service: mongos-service
