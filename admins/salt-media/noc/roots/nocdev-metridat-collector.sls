include:
  - units.mondata
  - units.metridat-collector
  - units.juggler-checks.common

metridat-collector:
  pkg.installed:
    - pkgs:
      - metridat-collector

yandex-unified-agent:
  pkg.installed: []
  service.running:
    - name: unified-agent
    - enable: True
    - reload: True
    - watch:
        - file: /etc/yandex/unified_agent*
