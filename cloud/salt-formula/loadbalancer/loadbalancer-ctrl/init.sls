yc-loadbalancer-ctrl:
  yc_pkg.installed:
    - pkgs:
      - yc-loadbalancer-ctrl
  service.running:
    - enable: True
    - watch:
      - file: /etc/yc/loadbalancer-ctrl/config.yaml
      - file: /etc/yc/loadbalancer-ctrl/discovery.yaml
      - yc_pkg: yc-loadbalancer-ctrl

/etc/yc/loadbalancer-ctrl/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - require:
      - yc_pkg: yc-loadbalancer-ctrl

/etc/yc/loadbalancer-ctrl/discovery.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/discovery.yaml.j2
    - template: jinja
    - require:
      - yc_pkg: yc-loadbalancer-ctrl
