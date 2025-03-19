
/opt/yandex/certificator-mock/etc/pki/CA.key:
    file.managed:
        - contents_pillar: data:certificator_mock:private
        - mode: 600
        - makedirs: True

/opt/yandex/certificator-mock/etc/pki/CA.pem:
    file.managed:
        - contents_pillar: data:certificator_mock:cert
        - mode: 600
        - makedirs: True

/opt/yandex/certificator-mock/etc/pki/crl:
    file.managed:
        - contents_pillar: data:certificator_mock:crl
        - mode: 600
        - makedirs: True


certificator-mock-container:
    docker_container.running:
        - name: certificator-mock
        - image: registry.yandex.net/cloud/dbaas/certificator-mock:v1.1-8160670
        - auto_remove: True
        - network_mode: "bridge"
        - binds: /opt/yandex/certificator-mock/etc/pki/:/config/pki/
        - port_bindings:
            - {{ salt['pillar.get']('data:certificator-mock:port', 8083) }}:80
        - require:
            - file: /opt/yandex/certificator-mock/etc/pki/CA.key
            - file: /opt/yandex/certificator-mock/etc/pki/CA.pem
            - file: /opt/yandex/certificator-mock/etc/pki/crl
