ntp:
  yc_pkg.installed:
    - pkgs:
      - ntp
  file.managed:
    - name: /etc/ntp.conf
    - source: salt://{{ slspath }}/files/ntp.conf
    - require:
      - yc_pkg: ntp
  service.running:
    - enable: True
    - require:
      - yc_pkg: ntp
      - file: ntp
    - watch:
      - file: ntp
