/etc/default/config-caching-dns:
    file.managed:
        - source: salt://{{ slspath }}/default
        - mode: 0644
        - user: root
        - group: root
