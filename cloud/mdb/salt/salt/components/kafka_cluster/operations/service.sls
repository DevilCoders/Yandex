include:
    - components.kafka.config
    - components.kafka.service
{% if salt['pillar.get']('service-restart') %}
    - components.kafka.restart
{% endif %}
