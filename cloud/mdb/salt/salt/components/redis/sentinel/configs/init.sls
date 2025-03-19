include:
    - .sentinel-config

extend:
    sentinel-config:
        file.managed:
            - mode: 640
            - makedirs: True
            - dir_mode: 750
            - require:
                - pkg: redis-salt-module-requirements
