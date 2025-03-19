data:
    internal_api:
        server_name: api-admin-dbaas01k.db.yandex.net
        config:
            identity:
                override_folder: fooc4olksjc12c27th2j
                override_cloud: fooubdgo8ls21u7u4enm
                create_missing: True
        tls_key: |
            {{ salt.yav.get('ver-01e0ahxdz3sreevbb0a6pm45ne[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01e0ahxdz3sreevbb0a6pm45ne[cert]') | indent(12) }}
