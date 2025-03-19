include:
{% if salt.pillar.get('data:mdb_metrics:enabled', True) %}
    - .mdb-metrics
{% else %}
    - components.mdb-metrics.disable
{% endif %}
    - .yasmagent
    - components.monrun2.kafka
    - components.pushclient2
    - .pushclient
