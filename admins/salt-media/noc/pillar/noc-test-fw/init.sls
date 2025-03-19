include:
  - units.yanet-logs
  - units.yanet-agent
  - units.yanet-solomon-agent

{% import_yaml slspath + '/autodeploy.yaml' as autodeploy_payload %}
salt-autodeploy: {{autodeploy_payload|json}}
salt-autodeploy-sls:
  units.percent:
  units.yanet-agent:
      monitor: True
