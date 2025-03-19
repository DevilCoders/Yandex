yc-microcosm:
  yc_pkg.installed:
    - pkgs:
      - yc-microcosm
  service.running:
    - enable: True
    - name: yc-microcosm
    - watch:
      - yc_pkg: yc-microcosm
      - file: /etc/yc/yc-microcosm/config.env
      - file: /etc/yc/yc-microcosm/config.yaml

/etc/yc/yc-microcosm/config.env:
  file.managed:
    - source: salt://{{ slspath }}/files/config.env
    - template: jinja
    - makedirs: True
    - dir_mode: 755
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-microcosm
      - file: /etc/yc/yc-microcosm

/etc/yc/yc-microcosm/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - makedirs: True
    - dir_mode: 755
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-microcosm
      - file: /etc/yc/yc-microcosm
