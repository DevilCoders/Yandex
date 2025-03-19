include:
  - components.logrotate
  # - components.monrun2.elasticsearch
  - components.monrun2.kibana
{% if salt.pillar.get('data:ship_logs', False) %}
  - components.pushclient2
  - .pushclient
{% endif %}
{% if salt.pillar.get('data:mdb_metrics:enabled', True) %}
  - .mdb-metrics
  - .yasmagent
{% else %}
  - components.mdb-metrics.disable
{% endif %}
  - .nginx
  - .opensearch
  - .dashboards
{% if salt.dbaas.is_dataplane() %}
  - .resize
{% endif %}

extend:
    dashboards-restart-service-req:
        test.nop:
            - require:
                - test: opensearch-service-ready
