/etc/vasgen/vasgen.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/vasgen.yaml
    - makedirs: True

vasgen:
  pkg.installed:
    - version: 1.0.0-9327286
    - allow_updates: True
  service.running:
    - enable: True
    - restart: True
    - wait3: True
    - watch:
      - file: /etc/vasgen/vasgen.yaml
      - pkg: vasgen
