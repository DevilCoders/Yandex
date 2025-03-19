include:
  - nginx

yc-graphics:
  yc_pkg.installed:
    - pkgs:
      - yc-graphics
  service.running:
    - enable: True
    - require:
      - yc_pkg: yc-graphics
      - file: /etc/yc/graphics/config.yaml
    - watch:
      - file: /etc/yc/graphics/config.yaml

/etc/yc/graphics/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/config.yaml
    - template: jinja
    - require:
      - yc_pkg: yc-graphics
