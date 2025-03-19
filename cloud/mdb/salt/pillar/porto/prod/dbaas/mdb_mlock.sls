data:
    mlock:
        mlockdb:
            password: {{ salt.yav.get('ver-01edcq93f885zt729cn4sxmny2[password]') }}
    mlock-cli:
        addr: '[::1]:443'
        config:
            security:
                insecure: false
                tls:
                    ca_file: /opt/yandex/allCAs.pem
                    server_name: mlock.db.yandex.net
        jwt:
            addr: ts.cloud.yandex-team.ru:4282
            id: fooa0riaitbmvt0vgg74
            key_id: f6ob92ph9tlcv1js5a9c
            private_key: |
                {{ salt.yav.get('ver-01ef4c56s1280tyhrhpcfxg2w1[private_key]') | indent(16) }}
        log_level: Fatal
