data:
    mlock:
        mlockdb:
            password: {{ salt.yav.get('ver-01edx46v7a88gjw9r6safvc2he[password]') }}
    mlock-cli:
        addr: '[::1]:443'
        config:
            security:
                insecure: false
                tls:
                    ca_file: /opt/yandex/allCAs.pem
                    server_name: mlock.private-api.yandexcloud.net
        jwt:
            addr: ts.private-api.cloud.yandex.net:4282
            id: aje9vkrjel7ndukgt1db
            key_id: ajeqbr8bvbd18hgvnuo2
            private_key: |
                {{ salt.yav.get('ver-01edx3x05r4163vek1vn78s9q5[private_key]') | indent(16) }}
        log_level: Fatal
