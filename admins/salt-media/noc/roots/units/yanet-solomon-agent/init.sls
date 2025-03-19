/etc/yanet-solomon-agent/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - makedirs: True

yanet-solomon-agent:
  pkg.installed:
    - version: 1.0.0-9531201
    - allow_updates: True
  service.running:
    - enable: True
    - restart: True
    - wait3: True
    - init_delay: 3
    - watch:
      - file: /etc/yanet-solomon-agent/config.yaml
      - pkg: yanet-solomon-agent
