mdb-kafka-s3-sink-connector-pkgs:
    pkg.installed:
        - pkgs:
            - kafka-connector-s3-sink: 1-2.12.0
        - require_in:
            - service: mdb-kafka-connect-worker-service
