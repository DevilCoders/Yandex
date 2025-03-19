yc-healthcheck-ctrl:
  yc_pkg.installed:
    - pkgs:
      - yc-healthcheck-ctrl

  service.running:
    - enable: True
    - require:
      - cmd: populate-database
    - watch:
      - file: /etc/yc/healthcheck-ctrl/config.yaml
      - file: /etc/yc/healthcheck-ctrl/discovery.yaml
      - yc_pkg: yc-healthcheck-ctrl

/etc/yc/healthcheck-ctrl/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - require:
      - yc_pkg: yc-healthcheck-ctrl

/etc/yc/healthcheck-ctrl/discovery.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/discovery.yaml.j2
    - template: jinja
    - require:
      - yc_pkg: yc-healthcheck-ctrl

populate-database:
  cmd.run:
    - name: /usr/bin/yc-healthcheck-ctrl db -cu --config /etc/yc/healthcheck-ctrl/config.yaml
    - require:
      - file: /etc/yc/healthcheck-ctrl/config.yaml
    - onchanges:
      - yc_pkg: yc-healthcheck-ctrl
