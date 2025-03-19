yandex-unified-agent:
  pkg.installed: []
  service.running:
    - name: unified-agent
    - enable: True
    - watch:
        - file: /etc/yandex/unified_agent*
