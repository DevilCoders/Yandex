data:
    config:
        metadb:
            hosts:
                - meta-test01f.db.yandex.net
                - meta-test01h.db.yandex.net
                - meta-test01k.db.yandex.net
            port: 6432
            user: idm_service
            dbname: dbaas_metadb
        crypto:
            api_sec_key: {{ salt.yav.get('ver-01dzb6cytp1gc4nrcdzf5ktse3[secret]') }}
            client_pub_key: {{ salt.yav.get('ver-01dzb6cytp1gc4nrcdzf5ktse3[public]') }}
    tls_key: |
        {{ salt.yav.get('ver-01e1esgp7rm5ncrryhpndgh9dx[private]') | indent(8) }}
    tls_crt: |
        {{ salt.yav.get('ver-01e1esgp7rm5ncrryhpndgh9dx[cert]') | indent(8) }}
