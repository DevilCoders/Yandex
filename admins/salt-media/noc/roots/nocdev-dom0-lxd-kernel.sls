include:
  - units.juggler-checks.common
  - units.juggler-checks.common_hw
  - units.dom0-common

/etc/sysctl.d:
  file.recurse:
    - source: salt://files/nocdev-dom0-lxd-kernel/etc/sysctl.d/

/etc/modules-load.d:
  file.recurse:
    - source: salt://files/nocdev-dom0-lxd-kernel/etc/modules-load.d/
