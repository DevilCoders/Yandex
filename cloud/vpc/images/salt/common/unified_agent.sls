unified-agent-config:
  file.managed:
    - name: /etc/yandex/unified_agent/conf.d/metrics.yml
    - template: jinja
    - source: salt://{{ slspath }}/files/unified_agent/metrics.yml
    - makedirs: True

unified-agent:
  pkg.installed:
    - pkgs:
      - yandex-unified-agent
  service.running:
    - enable: True
