solomon-mock-container:
    docker_container.running:
        - name: solomon-mock
        - image: registry.yandex.net/cloud/dbaas/solomon-mock:v1.1-6696577
        - auto_remove: True
        - network_mode: "bridge"
        - port_bindings:
            - {{ salt['pillar.get']('data:solomon-mock:port', 8082) }}:80
