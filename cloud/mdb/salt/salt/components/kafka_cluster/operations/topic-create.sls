{% set name = salt['pillar.get']('target-topic') %}
{% set topic = salt['pillar.get']('data:kafka:topics:' + name) %}

{% if topic %}
create-topic-{{ name|yaml_encode }}:
  mdb_kafka.topic_exists:
    - name: {{ name|yaml_encode }}
    - partitions: {{ topic.get('partitions') }}
    - replication_factor: {{ topic.get('replication_factor') }}
    - config: {{ topic.get('config', {}) }}
{% endif %}
