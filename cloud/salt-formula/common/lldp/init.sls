lldp-packages:
  yc_pkg.installed:
    - pkgs:
      - lldpd

/etc/lldpd.d/interfaces.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/interfaces.conf
    - makedirs: True

lldpd:
  service.running:
    - enable: True
    - watch:
      - file: /etc/lldpd.d/interfaces.conf
    - require:
      - yc_pkg: lldp-packages
