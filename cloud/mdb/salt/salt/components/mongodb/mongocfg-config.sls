/etc/mongodb/mongocfg.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/templates/mongocfg.conf
        - mode: 644
        - makedirs: True
