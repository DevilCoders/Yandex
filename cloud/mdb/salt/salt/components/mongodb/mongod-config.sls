/etc/mongodb/mongodb.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/templates/mongod.conf
        - mode: 644
        - makedirs: True
