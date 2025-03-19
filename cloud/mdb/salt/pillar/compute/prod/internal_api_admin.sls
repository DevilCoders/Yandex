data:
    internal_api:
        use_arcadia_build: true
        server_name: api-admin-dbaas01k.yandexcloud.net
        config:
            apispec_swagger_url: '/swagger/'
            apispec_swagger_ui_url: '/'
            identity:
                override_folder: b1g0r9fh49hee3rsc0aa
                override_cloud: b1ggh9onj7ljr7m9cici
                create_missing: False
        tls_key: |
            {{ salt.yav.get('ver-01e8vkndgs1x3w487f5v58v78q[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01e8vkndgs1x3w487f5v58v78q[cert]') | indent(12) }}
