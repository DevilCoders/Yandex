/etc/kafka/server.properties:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/server.properties
        - makedirs: True
        - mode: 644
