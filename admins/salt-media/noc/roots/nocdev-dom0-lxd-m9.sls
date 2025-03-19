include:
  - units.juggler-checks.common
  - units.juggler-checks.common_hw
  - units.dom0-common

/usr/share/yandex-hbf-agent/rules.d/06-internet.v6:
  file.managed:
    - source: salt://files/nocdev-dom0-lxd-ix/06-internet.v6
    - template: jinja
