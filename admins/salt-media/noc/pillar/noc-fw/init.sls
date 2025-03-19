{% import_yaml slspath + '/autodeploy.yaml' as autodeploy_payload %}
salt-autodeploy: {{autodeploy_payload|json}}
salt-autodeploy-sls:
  units.percent:
  units.yanet-agent:
      monitor: True

include:
  - units.yanet-logs
  - units.yanet-agent
  - units.yanet-solomon-agent

is_alive_systemd_services:
  - name: yanet-agent
    crit_message: maintenance enable

hw_watcher:
  enable_module:
    # - link - выключаем по задаче NOCDEV-6172
    - disk
    - mem
    - ecc
    - bmc
