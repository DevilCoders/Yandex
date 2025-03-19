{% set name = salt['pillar.get']('target-connector') %}

resume-connector-{{ name|yaml_encode }}:
  mdb_kafka_connect.connector_running:
    - name: {{ name|yaml_encode }}
