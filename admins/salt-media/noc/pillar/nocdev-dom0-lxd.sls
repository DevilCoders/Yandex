cluster: nocdev-dom0-lxd

include:
  - units.dom0-common

nginx:
  lookup:
    enabled: True
    log_params:
      name: 'nocdev-dom0-lxd-access-log'
      access_log_name: access.log

hw_watcher:
  bot:
    token: {{ salt.yav.get('sec-01epynddbcmerbgpr0rmgm2y5n[robot-nocdev-hw_oauth-token]') | json }}
