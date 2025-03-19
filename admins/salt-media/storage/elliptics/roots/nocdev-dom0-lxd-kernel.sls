include:
  - units.dom0-common-noc

/etc/sysctl.d:
  file.recurse:
    - source: salt://files/nocdev-dom0-lxd-kernel/etc/sysctl.d/

