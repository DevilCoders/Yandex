/opt/yandex/blackbox-mock/etc/config.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/config.py
        - template: jinja
        - makedirs: True

blackbox-mock-container:
    docker_container.running:
        - name: blackbox-mock
        - image: registry.yandex.net/cloud/dbaas/blackbox-mock:v1.1-6438473
        - auto_remove: True
        - binds: /opt/yandex/blackbox-mock/etc/config.py:/opt/yandex/config.py
        - network_mode: "bridge"
        - port_bindings:
            - {{ salt['pillar.get']('data:blackbox-mock:port', 8080) }}:80
        - require:
            - pkg: docker-pkgs
            - file: /opt/yandex/blackbox-mock/etc/config.py
        - watch:
            - file: /opt/yandex/blackbox-mock/etc/config.py
