data:
    mlock:
        mlockdb:
            password: {{ salt.yav.get('ver-01ecsx50z8caqd9pdx3x7dk2tg[password]') }}
    mlock-cli:
        addr: '[::1]:443'
        config:
            security:
                insecure: false
                tls:
                    ca_file: /opt/yandex/allCAs.pem
                    server_name: mlock.private-api.cloud-preprod.yandex.net
        jwt:
            addr: ts.private-api.cloud-preprod.yandex.net:4282
            id: bfbei0rfstrf9kf32tbq
            key_id: bfb5hf1sa2pl5cof9hl0
            private_key: |
                {{ salt.yav.get('ver-01ed45z4j1g7vrj814g6eswcrc[private_key]') | indent(16) }}
        log_level: Fatal
