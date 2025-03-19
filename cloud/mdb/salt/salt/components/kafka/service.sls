mdb-kafka-service:
    service.running:
        - name: kafka
        - enable: True
        - init_delay: 3
