{% import_yaml slspath + '/autodeploy.yaml' as autodeploy_payload %}
salt-autodeploy: {{autodeploy_payload|json}}
salt-autodeploy-sls:
  units.percent:
  units.yanet-agent:
      monitor: True

include:
  - units.yanet-agent
  - units.yanet-logs
  - units.yanet-solomon-agent
  - units.hw_watcher-paranoid

is_alive_systemd_services:
  - name: yanet-agent
    crit_message: maintenance enable
