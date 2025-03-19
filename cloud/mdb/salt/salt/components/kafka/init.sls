include:
    - .main
    - components.logrotate
    - .logrotate
    - .adminapi-gateway
{% if salt.pillar.get('data:kafka:sync_topics') %}
    - .kafka-agent
{% endif %}
{% if salt.pillar.get('data:kafka:connect_enabled', False) %}
    - components.kafka_connectors
    - .kafka-connect
{% endif %}
    - components.kafka_authorizer
{% if salt.pillar.get('data:kafka:schema_registry_enabled', False) %}
    - components.nginx
    - components.kafka.karapace
{% endif %}

{% if salt.dbaas.is_aws() %}
    - .init-aws
{% else %}
    - .init-yandex
{% endif %}

{% if salt.pillar.get('data:dbaas:cluster') %}
    - .resize
{% endif %}
