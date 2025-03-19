{% set name = salt['pillar.get']('target-topic') %}

drop-topic-{{ name|yaml_encode }}:
  mdb_kafka.topic_absent:
    - name: {{ name|yaml_encode }}
