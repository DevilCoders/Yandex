BIND packages:
  pkg.installed:
  - pkgs:
    - bind9

BIND configuration:
  file.managed:
    - name: /etc/bind/named.conf.options
    - source: salt://{{ slspath }}/files/named.conf.options
