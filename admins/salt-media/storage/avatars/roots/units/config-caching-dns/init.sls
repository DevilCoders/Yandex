/etc/default/config-caching-dns:
  file.managed:
    - source: salt://files/config-caching-dns/default
    - mode: 644
    - user: root
    - group: root
