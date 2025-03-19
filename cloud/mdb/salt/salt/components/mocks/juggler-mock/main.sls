juggler-mock-container:
    docker_container.running:
        - name: juggler-mock
        - image: registry.yandex.net/cloud/dbaas/juggler-mock:v1.1-7322010
        - auto_remove: True
        - network_mode: "bridge"
        - port_bindings:
            - {{ salt['pillar.get']('data:juggler-mock:port', 8081) }}:80
