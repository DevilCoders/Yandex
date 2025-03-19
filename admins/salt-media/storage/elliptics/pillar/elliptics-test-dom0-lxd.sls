cluster: elliptics-test-dom0-lxd

include:
  - units.dom0-common

nginx:
  lookup:
    enabled: true
    log_params:
      name: 'elliptics-test-dom0-lxd-access-log'
      access_log_name: access.log
