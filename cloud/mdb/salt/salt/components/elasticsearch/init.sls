include:
  - components.logrotate
  - components.monrun2.elasticsearch
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
  - .elasticsearch
  - .kibana
{% if salt.dbaas.is_dataplane() %}
  - .resize
{% endif %}

extend:
    kibana-restart-service-req:
        test.nop:
            - require:
                - test: elasticsearch-service-ready
