{% set name = salt['pillar.get']('target-connector') %}

pause-connector-{{ name|yaml_encode }}:
  mdb_kafka_connect.connector_paused:
    - name: {{ name|yaml_encode }}
