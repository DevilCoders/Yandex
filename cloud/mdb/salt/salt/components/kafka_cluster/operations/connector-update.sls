{% set name = salt['pillar.get']('target-connector') %}
{% set connector_data = salt['pillar.get']('data:kafka:connectors:' + name) %}

{% if connector_data %}
update-connector-{{ name|yaml_encode }}:
  mdb_kafka_connect.connector_exist:
    - name: {{ name|yaml_encode }}
    - connector_data: {{ connector_data }}
{% endif %}
