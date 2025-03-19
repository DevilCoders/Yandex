/etc/mongodb/mongos.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/templates/mongos.conf
        - mode: 644
        - makedirs: True
