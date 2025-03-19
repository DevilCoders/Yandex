include:
  - units.mondata
  - units.metridat-client
  - units.juggler-checks.common

metridat-client:
  pkg.installed:
    - pkgs:
      - metridat-client
  service.running:
    - enable: True

yandex-unified-agent:
  pkg.installed: []
  service.running:
    - name: unified-agent
    - enable: True
    - reload: True
    - watch:
        - file: /etc/yandex/unified_agent*
