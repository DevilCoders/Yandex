yc-healthcheck-node:
  yc_pkg.installed:
    - pkgs:
      - yc-healthcheck-node

  service.running:
    - enable: True
    - require:
      - file: /etc/yc/healthcheck-node/config.yaml
      - yc_pkg: yc-healthcheck-node
    - watch:
      - file: /etc/yc/healthcheck-node/config.yaml
      - yc_pkg: yc-healthcheck-node

/etc/yc/healthcheck-node/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - require:
      - yc_pkg: yc-healthcheck-node
