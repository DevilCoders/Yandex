atop:
  yc_pkg.installed:
    - pkgs:
      - atop
  file.managed:
    - name: /etc/default/atop
    - source: salt://{{ slspath }}/files/atop
    - require:
      - yc_pkg: atop
  service.running:
    - enable: True
    - require:
      - yc_pkg: atop
      - file: atop
    - watch:
      - file: atop
