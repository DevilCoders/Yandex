data:
    internal_api:
        server_name: api-admin-dbaas-preprod01f.cloud-preprod.yandex.net
        config:
            apispec_swagger_url: '/swagger/'
            apispec_swagger_ui_url: '/'
            identity:
                override_cloud: aoe9shbqc2v314v7fp3d
                override_folder: aoed5i52uquf5jio0oec
                create_projects: False
        tls_key: |
            {{ salt.yav.get('ver-01dw7seb29tvbh2d23kxxx7ndx[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01dw7seb29tvbh2d23kxxx7ndx[cert]') | indent(12) }}
