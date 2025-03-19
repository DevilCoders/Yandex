yc-loadbalancer-node:
  yc_pkg.installed:
    - pkgs:
      - yc-loadbalancer-node
  service.running:
    - enable: True
    - watch:
      - file: /etc/yc/loadbalancer-node/config.yaml
      - yc_pkg: yc-loadbalancer-node

vpp:
  group.present:
    - addusers:
      - yc-loadbalancer-node
    - require:
      - yc_pkg: yc-loadbalancer-node
      - yc_pkg: vpp


/etc/yc/loadbalancer-node/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - require:
      - yc_pkg: yc-loadbalancer-node
