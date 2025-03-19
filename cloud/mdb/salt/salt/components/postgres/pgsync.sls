/etc/pgsync.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/pgsync.conf
        - mode: 644
        - user: root
        - group: root

pgsync:
    service.running:
        - watch:
            - file: /etc/pgsync.conf
