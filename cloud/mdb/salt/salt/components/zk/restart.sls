zookeeper-restart:
    cmd.run:
        - name: >
            service zookeeper restart
        - require_in:
            - service: zookeeper-service
