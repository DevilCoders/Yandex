{% set name = salt['pillar.get']('target-connector') %}

mdb-kafka-delete-connector-{{ name|yaml_encode }}:
    mdb_kafka_connect.connector_absent:
        - name: {{ name|yaml_encode }}
