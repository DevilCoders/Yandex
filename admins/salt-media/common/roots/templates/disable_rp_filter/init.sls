/etc/sysctl.d/ZZ-rp-filter.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - source: salt://{{slspath}}/files/ZZ-rp-filter.conf
