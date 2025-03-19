data:
    config:
        metadb:
            hosts:
                - meta01h.db.yandex.net
                - meta01f.db.yandex.net
                - meta01k.db.yandex.net
            port: 6432
            user: idm_service
            dbname: dbaas_metadb
        crypto:
            api_sec_key: {{ salt.yav.get('ver-01dzb6cytp1gc4nrcdzf5ktse3[secret]') }}
            client_pub_key: {{ salt.yav.get('ver-01dzb6cytp1gc4nrcdzf5ktse3[public]') }}
