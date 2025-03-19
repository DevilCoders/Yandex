{% set username = salt['pillar.get']('target-user') %}

mdb-kafka-delete-user-{{ username|yaml_encode }}:
    mdb_kafka.users_sync:
        - username: {{ username|yaml_encode }}

{% if salt.pillar.get('data:kafka:schema_registry_enabled', False) %}
include:
    - components.kafka.karapace.refresh-config
{% endif %}
