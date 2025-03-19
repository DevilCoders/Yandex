data:
    sa_tokens:
        'https://iaas.private-api.cloud-preprod.yandex.net': {{ salt.yav.get('ver-01ew281j43dnpf77xmr6s3hp28[token]') }}
        'https://iaas.private-api.cloud.yandex.net': {{ salt.yav.get('ver-01e0qfxnt23hxxzs18ptwz00y5[token]') }}
    image_upload_config:
        preprod_control_plane:
            api_url: https://iaas.private-api.cloud-preprod.yandex.net
            identity_private_api_url: https://identity.private-api.cloud-preprod.yandex.net:14336
            identity_public_api_url: https://identity.private-api.cloud-preprod.yandex.net:14336
            oauth_token: {{ salt.yav.get('ver-01e0qfg7v7m8vfgr04va21e4qc[token]') }}
            intranet_clients:
                ssl_verify: /opt/yandex/CA.pem
            cloud: mdb
            folder: control-plane

        preprod_clusters:
            api_url: https://iaas.private-api.cloud-preprod.yandex.net
            identity_private_api_url: https://identity.private-api.cloud-preprod.yandex.net:14336
            identity_public_api_url: https://identity.private-api.cloud-preprod.yandex.net:14336
            oauth_token: {{ salt.yav.get('ver-01e0qfg7v7m8vfgr04va21e4qc[token]') }}
            intranet_clients:
                ssl_verify: /opt/yandex/CA.pem
            cloud: mdb
            folder: clusters

        prod_clusters:
            api_url: https://iaas.private-api.cloud.yandex.net
            identity_private_api_url: https://identity.private-api.cloud.yandex.net:14336
            identity_public_api_url: https://identity.private-api.cloud.yandex.net:14336
            oauth_token: {{ salt.yav.get('ver-01e0qfg7v7m8vfgr04va21e4qc[token]') }}
            intranet_clients:
                ssl_verify: /opt/yandex/CA.pem
            cloud: mdb
            folder: clusters

        prod_control_plane:
            api_url: https://iaas.private-api.cloud.yandex.net
            identity_private_api_url: https://identity.private-api.cloud.yandex.net:14336
            identity_public_api_url: https://identity.private-api.cloud.yandex.net:14336
            oauth_token: {{ salt.yav.get('ver-01e0qfg7v7m8vfgr04va21e4qc[token]') }}
            intranet_clients:
                ssl_verify: /opt/yandex/CA.pem
            cloud: mdb
            folder: control-plane
