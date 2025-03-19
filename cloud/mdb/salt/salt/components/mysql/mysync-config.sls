/etc/mysync.yaml:
    file.managed:
        - user: mysql
        - group: mysql
        - template: jinja
        - source: salt://{{ slspath }}/conf/mysync.yaml
        - mode: 640

mysync:
    service.running:
        - reload: False
        - watch:
            - file: /etc/mysync.yaml
        - require:
            - file: /etc/mysync.yaml
