/etc/vector/vector.toml:
    file.managed:
        - source: salt://{{ slspath }}/conf/vector.toml
        - mode: 640
        - user: vector
        - template: jinja
        - require:
            - pkg: vector-package
        - watch_in:
            - service: vector-service

/etc/vector/test_vector.toml:
    file.managed:
        - source: salt://{{ slspath }}/conf/test_vector.toml
        - mode: 640
        - user: vector
        - template: jinja
        - require:
            - pkg: vector-package
        - watch_in:
            - service: vector-service

