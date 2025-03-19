mongod-service:
    service.running:
        - name: mongodb
        - enable: True
        - reload: True

    mdb_mongodb.alive:
        - tries: 10
        - timeout: 5
        - service: mongod
        - use_auth: False
        - watch:
            - service: mongod-service
