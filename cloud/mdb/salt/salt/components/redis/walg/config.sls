/etc/wal-g/wal-g.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g.yaml
        - user: root
        - group: s3users
        - makedirs: True
        - mode: 640
